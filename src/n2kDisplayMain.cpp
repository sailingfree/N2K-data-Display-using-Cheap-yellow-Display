/*

 This device reads NMEA2000 messages over WiFi in YD format and uses those to display
 the required data.

 The source of YD messages in my system is the ESP32 WiFi gateway

Copyright (c) 2022 Peter Martin www.naiadhome.com

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <YDtoN2KUDP.h>
#include <ESPmDNS.h>
#include <GwTelnet.h>

// NMEA headers
#include <N2kMessages.h>
#include <N2kMsg.h>
#include <NMEA0183.h>
#include <NMEA0183Messages.h>
#include <NMEA0183Msg.h>
#include <NMEA2000.h>
#include <NMEA2000_CAN.h>  // This will automatically choose right CAN library and create suitable NMEA2000 object

// HTML strings
#include <style.html>       // Must come before the content files
#include <login.html>
#include <server_index.html>

// Admin
#include <GwPrefs.h>
#include <GwShell.h>
#include <SimpleSerialShell.h>
#include <StringStream.h>
#include <SysInfo.h>
#include <map>
#include <Preferences.h>
#include <GwOTA.h>

// Display 
#include <tftscreen.h>

// The web server
WebServer server(80);

// The UDP yacht data reader
YDtoN2kUDP ydtoN2kUDP;

// The wifi UDP socket
WiFiUDP wifiUdp;

// The telnet server for the shell.
WiFiServer telnet(23);

// Telnet client for connections
WiFiClient telnetClient;

// Global objects and variables
String host_name;
String Model = "Naiad Display 1";
String macAddress;              // Make the mac address gloabal
Stream *OutputStream = NULL;  //&Serial;

// Define the console to output to serial at startup.
// this can get changed later, eg in the gwshell.
Stream *Console = &Serial;

// Map for received n2k messages. Logs the PGN and the count
std::map<int, int> N2kMsgMap;

// Wifi mode
typedef enum { WiFi_off,
               WiFi_AP,
               WiFi_Client } Gw_WiFi_Mode;

// Map for the wifi access points
typedef struct {
    String ssid;
    String pass;
} WiFiCreds;

static const uint16_t MaxAP = 2;
WiFiCreds wifiCreds[MaxAP];

// Wifi cofiguration Client and Access Point
String AP_password;  // AP password  read from preferences
String AP_ssid;      // SSID for the AP constructed from the hostname

// Put IP address details here
const IPAddress AP_local_ip(192, 168, 15, 1);   // Static address for AP
const IPAddress AP_gateway(192, 168, 15, 1);    // AP is the gateway
const IPAddress AP_subnet(255, 255, 255, 0);    // /24 subnet

// This application will only ever be a client so predefine this here
Gw_WiFi_Mode wifiType = WiFi_AP;

String WifiMode = "Unknown";
String WifiSSID = "Unknown";
String WifiIP = "Unknown";

// Setup the shell and other admin
void adminSetup(void) {
    uint8_t chipid[6];
    uint32_t id = 0;
    uint32_t i;

    // get the MAC address
    esp_err_t fuse_error = esp_efuse_mac_get_default(chipid);
    if (fuse_error) {
        Serial.printf("efuse error: %d\n", fuse_error);
    }

    for (i = 0; i < 6; i++) {
        if (i != 0) {
            macAddress += ":";
        }
        id += (chipid[i] << (7 * i));
        macAddress += String(chipid[i], HEX);
    }

       // Initialise the preferences
    GwPrefsInit();

    // Generate the hostname by appending the last two octets of the mac address to make it unique
    String hname = GwGetVal(GWHOST, "n2kdisplay");
    host_name = hname + String(chipid[4], HEX) + String(chipid[5], HEX);
        
    // Init the shell
    initGwShell();
    setShellSource(&Serial);
}

void adminWork() {
        if(WiFi.status() == WL_CONNECTED) {
    // handle any telnet sessions
    handleTelnet();
        }

    //run any shell commands
    handleShell();

    if(WiFi.status() == WL_CONNECTED) {
    // Handle any OTA requests
    handleOTA();
    }
}


// Web server
void webServerSetup(void) {
        if(WiFi.status() == WL_CONNECTED) {

    Serial.println("Web server started");
    server.on("/", HTTP_GET, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", loginIndex);
    });
    server.on("/serverIndex", HTTP_GET, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", serverIndex);
    });
    /*handling uploading firmware file */
    server.on("/", HTTP_GET, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", loginIndex);
    });
    server.on("/serverIndex", HTTP_GET, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", serverIndex);
    });
    /*handling uploading firmware file */
    server.on(
        "/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart(); }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    } });
    sleep(2);
    server.begin();
        }
}

void webServerWork() {
        if(WiFi.status() == WL_CONNECTED) {
        server.handleClient();
        }
}



// Connect to a wifi AP
// Try all the configured APs
static bool hadconnection = false;

bool connectWifi() {
    int wifi_retry = 0;

  //  Serial.printf("There are %d APs to try\n", MaxAP);

    for (int i = 0; i < MaxAP; i++) {
        if (wifiCreds[i].ssid != "---") {
            Serial.printf("\nTrying %s\n", wifiCreds[i].ssid.c_str());
            WiFi.disconnect();
            WiFi.mode(WIFI_OFF);
            WiFi.mode(WIFI_STA);
            
            // Do two begins here with a delay to get around the problem that after upload
            // the first wifi begin often fails.
            // From here https://www.esp32.com/viewtopic.php?t=12720
            WiFi.begin(wifiCreds[i].ssid.c_str(), wifiCreds[i].pass.c_str());
            delay(100);
            WiFi.begin(wifiCreds[i].ssid.c_str(), wifiCreds[i].pass.c_str());
            wifi_retry = 0;

            while (WiFi.status() != WL_CONNECTED && wifi_retry < 20) {  // Check connection, try 5 seconds
                wifi_retry++;
                delay(500);
                Console->print(".");
            }
            Console->println("");
            if (WiFi.status() == WL_CONNECTED) {
                WifiMode = "Client";
                WifiSSID = wifiCreds[i].ssid;
                WifiIP = WiFi.localIP().toString();
                Console->printf("Connected to %s\n", wifiCreds[i].ssid.c_str());
                hadconnection = true;
                setilabel(WifiIP);
                return true;
            } else {
                Console->printf("Can't connect to %s\n", wifiCreds[i].ssid.c_str());
            }
        }
    }
    return false;
}

void disconnectWifi() {
    Console = &Serial;
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    WiFi.mode(WIFI_STA);
    WifiMode = "Not connected";
}

void wifiCheck() {
    uint32_t wifi_retry = 0;

    if (hadconnection) {
        while (WiFi.status() != WL_CONNECTED && wifi_retry < 5) {  // Connection lost, 5 tries to reconnect
            wifi_retry++;
            Console->println("WiFi not connected. Try to reconnect");
            disconnectWifi();
            connectWifi();
        }
    }
}

// WiFi setup.
// Connect to a wifi AP which supplies the data we need.
// Register services we use
void wifiSetup(void) {
    Serial.println("Starting WiFi manager task...");

    uint32_t retries = 5;

    // Get the configured wifi type
    String wifi_mode_string = GwGetVal(WIFIMODE, "0");  // 0 = off, 1 = ap, 2 = client
    wifiType = (Gw_WiFi_Mode)wifi_mode_string.toInt();

    // setup the WiFI map from the preferences
    wifiCreds[0].ssid = GwGetVal(SSID1);
    wifiCreds[0].pass = GwGetVal(SSPW1);
    wifiCreds[1].ssid = GwGetVal(SSID2);
    wifiCreds[1].pass = GwGetVal(SSPW2);

    while (!connectWifi() && --retries) {
        sleep(1);
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Failed to connect to WiFi please check creds");
    }
    else {
        Serial.println("WiFi connected..!");
        Serial.print("Got IP: ");
        Serial.println(WiFi.localIP());

        if (MDNS.begin(host_name.c_str())) {
            Console->print("* MDNS responder started. Hostname -> ");
            Console->println(host_name);
        }
        else {
            Console->printf("Failed to start the MDNS respondern");
        }

        MDNS.addService("http", "tcp", 80);  // Web server

        Console->println("Adding telnet");
        MDNS.addService("telnet", "tcp", 23);  // Telnet server of RemoteDebug, register as telnet

        // Start the telnet server
        telnet.begin();

        // start the YD UDP socket
        ydtoN2kUDP.begin(4444);

        // Start the OTA service
        initializeOTA(Console);
    }
}

// loop reading the YD data, decode the N2K messages 
// and update the screen copies.
void wifiWork(void) {
        tN2kMsg msg;

        if(WiFi.status() == WL_CONNECTED) {
                    ydtoN2kUDP.readYD(msg);

        N2kMsgMap[msg.PGN]++;
        switch (msg.PGN) {
            case 127508: {
                // Battery Status
                unsigned char instance = 0xff;
                double voltage = 0.0;
                double current = 0.0;
                double temp = 273.0;
                unsigned char SID = 0xff;
                bool s = ParseN2kPGN127508(msg, instance, voltage, current, temp, SID);

                switch (instance) {
                    case 0:
                        setMeter(HOUSEV, voltage);
                        setMeter(HOUSEI, current);
                        break;
                    case 1:
                        setMeter(ENGINEV, voltage);;
                        break;
                }
            } break;

            case 127513:
                // Not interested
                break;

            case 127488: {
                // Engine Rapid
                unsigned char instance;
                double speed;
                double boost;
                int8_t trim;
                bool s = ParseN2kPGN127488(msg, instance, speed, boost, trim);

                setMeter(RPM, speed);
                setMeter(3, speed);
            }
            
            default:
                // Catch any messages we don't expect
               break;
        }
        }
 }

// Initilise the modules
void setup() {
    // set up serial debug
    Serial.begin(115200);

    adminSetup();           // Should be called first to setup preferences etc
    metersSetup();          // Graphics setup
    wifiSetup();            // Conect to an AP for the YD data
    webServerSetup();       // remote management
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
}

// loop calling the work functions
void loop(void) {
    adminWork();
    wifiWork();
    webServerWork();
    metersWork();
    wifiCheck();
    delay(50);
}
