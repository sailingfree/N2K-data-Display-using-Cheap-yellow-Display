// General WiFI handling
#include <WiFi.h>

// Wifi mode
typedef enum { WiFi_off,
               WiFi_AP,
               WiFi_Client } Gw_WiFi_Mode;

// Map for the wifi access points
typedef struct {
    String ssid;
    String pass;
} WiFiCreds;

extern Stream * Console;

// External declarations
void wifiSetup(String & host_name);
void wifiCheck(void);
void wifiWork(void);
void webServerSetup(void);
void webServerWork();
