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

#include <GwTelnet.h>

// NMEA headers
#include <N2kMessages.h>
#include <N2kMsg.h>
#include <NMEA0183.h>
#include <NMEA0183Messages.h>
#include <NMEA0183Msg.h>
#include <NMEA2000.h>
#include <NMEA2000_CAN.h>  // This will automatically choose right CAN library and create suitable NMEA2000 object


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

// N2K PGN handlers
#include <handlePGN.h>

// WiFI handling
#include <MyWiFi.h>

// Global objects and variables
String host_name;
String Model = "Naiad Display 1";
String macAddress;              // Make the mac address gloabal
Stream *OutputStream = NULL;    //&Serial;

// Define the console to output to serial at startup.
// this can get changed later, eg in the gwshell.
Stream *Console = &Serial;


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
    if (WiFi.status() == WL_CONNECTED) {
        // handle any telnet sessions
        handleTelnet();
    }

    // run any shell commands
    handleShell();

    if (WiFi.status() == WL_CONNECTED) {
        // Handle any OTA requests
        handleOTA();
    }
}






// Initilise the modules
void setup() {
    // set up serial debug
    Serial.begin(115200);

    adminSetup();           // Should be called first to setup preferences etc
    metersSetup();          // Graphics setup
    wifiSetup(host_name);   // Conect to an AP for the YD data
    webServerSetup();       // remote management
    displayText("Web server started...");
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    // Finally load the first working screen
    loadScreen();
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
