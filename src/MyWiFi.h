// General WiFI handling for esp 32 projects
#include <WiFi.h>

// Wifi mode
typedef enum { WiFi_off,
               WiFi_AP,
               WiFi_Client } Gw_WiFi_Mode;

// Map for the wifi access points names and credentials
typedef struct {
    String ssid;
    String pass;
} WiFiCreds;

extern Stream * Console;

// External declarations

// Initialise the wifi network
void wifiSetup(String & host_name);

// Check its still connected and re-connect if not
void wifiCheck(void);

// Do some work with the network
void wifiWork(void);

// Init the web server.
void webServerSetup(void);

// Do some work with the web server
void webServerWork();
