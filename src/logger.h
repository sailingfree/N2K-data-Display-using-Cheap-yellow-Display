// Log data to the sdcard

#include <Arduino.h>
#include <stdio.h>

#define LOG_HOUSE_VOLTAGE   "hvolts"
#define LOG_HOUSE_CURRENT   "hcurrent"
#define LOG_ENGINE_VOLTAGE  "evolts"
#define LOG_RPM             "rpm"



FILE *  openLog(String logname);
void logClose(FILE *);
void logAdd(FILE *, String & msg);