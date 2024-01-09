// Common wifi handling

#include <ESPmDNS.h>
#include <GwOTA.h>
#include <GwPrefs.h>
#include <MyWiFi.h>
#include <StringStream.h>
#include <WebServer.h>
#include <YDtoN2KUDP.h>
#include <handlePGN.h>
#include <tftscreen.h>

#include <map>

// HTML strings
#include <html/style.html>  // Must come before the content files
#include <html/login.html>
#include <html/server_index.html>

// Map for received n2k messages. Logs the PGN and the count
std::map<int, int> N2kMsgMap;

// Storage for some wifi credentials.
static const uint16_t MaxAP = 2;
WiFiCreds wifiCreds[MaxAP];

// Wifi cofiguration Client and Access Point
String AP_password;  // AP password  read from preferences
String AP_ssid;      // SSID for the AP constructed from the hostname

// IP network address when working in AP mode
const IPAddress AP_local_ip(192, 168, 15, 1);  // Static address for AP
const IPAddress AP_gateway(192, 168, 15, 1);   // AP is the gateway
const IPAddress AP_subnet(255, 255, 255, 0);   // /24 subnet

// This application will only ever be a client so predefine this here
Gw_WiFi_Mode wifiType = WiFi_AP;

String WifiMode = "Unknown";
String WifiSSID = "Unknown";
String WifiIP = "Unknown";

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

// Connect to a wifi AP
// Try all the configured APs
static bool hadconnection = false;

bool connectWifi() {
    int wifi_retry = 0;

    //  Serial.printf("There are %d APs to try\n", MaxAP);

    for (int i = 0; i < MaxAP; i++) {
        if (wifiCreds[i].ssid != "---") {
            StringStream s;
            s.printf("\nTrying %s\n", wifiCreds[i].ssid.c_str());
            Serial.print(s.data);
            displayText((char*)s.data.c_str());
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
                setilabel(SCR_ENGINE, WifiIP);
                setilabel(SCR_NAV, WifiIP);
                String msg("AP: ");
                msg += wifiCreds[i].ssid;
                msg += "\nIP: ";
                msg += WifiIP;
                displayText((char*)msg.c_str());
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
void wifiSetup(String& host_name) {
    Serial.println("Starting WiFi manager task...");
    displayText("Starting WiFi...");

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
        displayText("Can't connect to wifi please check creds");
    } else {
        Serial.println("WiFi connected..!");
        Serial.print("Got IP: ");
        Serial.println(WiFi.localIP());

        if (MDNS.begin(host_name.c_str())) {
            Console->print("* MDNS responder started. Hostname -> ");
            Console->println(host_name);
        } else {
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

// Web server
void webServerSetup(void) {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Web server started");
        displayText("Web Server started");
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
    if (WiFi.status() == WL_CONNECTED) {
        server.handleClient();
    }
}

// loop reading the YD data, decode the N2K messages
// and update the screen copies.
void wifiWork(void) {
    tN2kMsg msg;

    if (WiFi.status() == WL_CONNECTED) {
        while (ydtoN2kUDP.readYD(msg)) {
            N2kMsgMap[msg.PGN]++;
            handlePGN(msg);
        }
    }
}
