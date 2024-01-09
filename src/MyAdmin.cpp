#include <Arduino.h>
#include <GwPrefs.h>
#include <GwShell.h>
#include <MyWiFi.h>
#include <GwOTA.h>
#include <GwTelnet.h>

// Global objects and variables
String host_name;
String Model = "Naiad Display 1";
String macAddress;              // Make the mac address gloabal

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
        handleOTA();
    }

    // run any local shell commands
    handleShell();
}
