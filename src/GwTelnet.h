#include <Arduino.h>
#include <GwShell.h>
#include <WiFi.h>

extern WiFiServer telnet;
extern WiFiClient telnetClient;

void disconnect();
void handleTelnet();
