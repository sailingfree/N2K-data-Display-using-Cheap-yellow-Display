#include <Arduino.h>
#include <WiFi.h>
#include <GwShell.h>


extern WiFiServer   telnet;
extern WiFiClient   telnetClient;

void disconnect();
void handleTelnet();
