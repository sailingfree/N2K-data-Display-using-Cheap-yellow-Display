/*

 Replacement instrument for a Simrad IS15 combi display.

 Uses an ESP32 and a TFT display to completely replace the inards of the IS15.
 The old IS15 stopped working completely and Simrad would not repair it or provide
 any details of the board for it to be repaired. 

 This device reads NMEA2000 messages over WiFi in YD format and uses those to display 
 the required data.

 The source in my system is the ESP32 WiFi gateway 

 */

#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include "Free_Fonts.h" // Include the header file attached to this sketch
#include <rotary_encoder.h>
#include <YDtoN2KUDP.h>

// NMEA headers
#include <N2kMessages.h>
#include <N2kMsg.h>
#include <NMEA0183.h>
#include <NMEA0183Messages.h>
#include <NMEA0183Msg.h>
#include <NMEA2000.h>
#include <NMEA2000_CAN.h>  // This will automatically choose right CAN library and create suitable NMEA2000 object


// Custom fonts
#include "FreeSansBold32pt7b.h"

// HTML strings
#include <style.html>
#include <login.html>
#include <server_index.html>

// Prototypes
void updateDigital(int idx);
void digitalMeter(int idx);
void runMeters();


// Instatiate required objects

// The TFT library
TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

// The web server
WebServer server(80);

// The UDP yacht data reader
YDtoN2kUDP ydtoN2kUDP;

// Define the quadrants
#define TFT_GREY 0x5AEB
#define IDX0  0
#define X0    0
#define Y0    0

#define IDX1  1
#define X1    TFT_HEIGHT/2
#define Y1    0

#define IDX2  2
#define X2    0
#define Y2    TFT_WIDTH/2

#define IDX3  3
#define X3    TFT_HEIGHT/2
#define Y3    TFT_WIDTH/2

// Margins
#define X_MARGIN 5
#define Y_MARGIN 2


typedef struct {
  double  value;
  uint32_t  x;
  uint32_t  y;
  char      type;
  String label;
  bool      doneLabel;
  uint32_t  oldx;
  uint32_t  oldw;
  double oldvalue;
} Digital;

Digital digital[] = {
  {1.0, X0, Y0, 'i'},
  {2.0, X1, Y1, 'i'},
  {3.0, X2, Y2, 'f'},
  {4.0, X3, Y3, 'f'},
};

uint32_t updateTime = 0;       // time for next update

String label[6] = {"idx0","idx1","idx2","idx3"};

WiFiUDP wifiUdp;


// Main task for plotting the meters using data previously read
// Draw the display elements then loop forever
void metersTask(void * param) {
  Serial.println("Starting the meter draw task...");
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  rotary_setup();

  digitalMeter(IDX0);       // Draw digitalue meter
  digitalMeter(IDX1);     // Draw digitalue meter
  digitalMeter(IDX2);     // Draw digitalue meter
  digitalMeter(IDX3);   // Draw digitalue meter

  updateTime = millis(); // Next update time
  Serial.println("Done setting up meters...");

  for(;;) {
    const TickType_t xDelay = 1000 / portTICK_PERIOD_MS;
    vTaskDelay(xDelay);
    runMeters();
  }
}



void runMeters() {
    updateDigital(IDX0);
    updateDigital(IDX1); 
    updateDigital(IDX2); 
    updateDigital(IDX3); 
}


// #########################################################################
//  Draw the digital meter on the screen
// #########################################################################
void digitalMeter(int idx)
{
  // Meter outline
  tft.fillRect(digital[idx].x, digital[idx].y, 239, 160, TFT_GREY);

  // 
  tft.fillRect(digital[idx].x + X_MARGIN, digital[idx].y+3, (TFT_HEIGHT / 2) - (2 * X_MARGIN), (TFT_WIDTH / 2) - Y_MARGIN, TFT_WHITE);

  tft.setTextColor(TFT_BLACK);  // Text colour

}
  
// #########################################################################
// Update the value on screen
// This function is blocking while we update the value
// #########################################################################
void updateDigital(int idx)
{
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  char buf[8]; 
  memset(buf, 0, sizeof(buf));

  double  value = digital[idx].value;
  int32_t vtype = digital[idx].type;
  int32_t xorig = digital[idx].x;
  int32_t yorig = digital[idx].y;

  if(value == digital[idx].oldvalue) {
    return;
  }
  digital[idx].oldvalue = value;
  switch(vtype) {
    case 'f':
    //dtostrf(value/1.0, 3, 1, buf);
    value += 0.2345;
    snprintf(buf, 7, "%3.2f", value);
//    Serial.printf("Value = %f %3.2f\n", value, value);
    break;

    case 'i':
    default:
      snprintf(buf, 6, "%d", (int)value);   // we can fit 4 digits per cell.
      break;
  }

// Draw the label one time only
 if(!digital[idx].doneLabel) {
  tft.drawCentreString(label[idx], xorig + TFT_HEIGHT / 4, yorig + 20, 4);
  digital[idx].doneLabel = true;
 }

  // Set the required font for the main text
  tft.setFreeFont(&FreeSansBold32pt7b);       // Select Free Serif 24 point font

  // Calculate the position of the text to simplify things later
  // The string is centered and drawn in the middle of the bottom half of the
  // box

  uint32_t  tw = tft.textWidth(buf, GFXFF);       // Width of text
  uint32_t  th = tft.fontHeight(GFXFF);           // height of text
  uint32_t  tx = xorig + TFT_HEIGHT / 4 - tw / 2;          // Position of bottom left
  uint32_t  ty = yorig + th;     
  uint32_t  oldx = digital[idx].oldx;
  uint32_t  oldw = digital[idx].oldw; 
  
  if(idx == 0) {
  //Serial.printf("tx %d ty %d\n", tx, ty);
  }
  // Blank the area where the new text is written using the font height and]
  // the previous width and x
 // tft.fillRect(xorig + X_MARGIN, yorig+height, 230, height, TFT_GREEN);
  tft.fillRect(oldx, ty, oldw, th, TFT_WHITE);
  digital[idx].oldx = tx;
  digital[idx].oldw = tw;
 
  // Update 
  tft.drawString(buf, tx, ty, GFXFF);
}



 

// Web server
void webServerTask(void * parm)
{
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
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
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
    }
  });
  sleep(2);
server.begin();
  for(;;) {
      server.handleClient();
      const TickType_t xDelay = 5 / portTICK_PERIOD_MS;
      vTaskDelay(xDelay);
  }
}


// manage the  wifi

const String hostname  = "damselfly1";    //Hostname for network discovery
const char * ssid      = "Damselfly4e54";     //SSID to connect to
const char * ssidPass  = "Hanse301";  // Password for wifi


// WiFi task.
void wifiManagerTask(void * param) 
{
  Serial.println("Starting WiFi manager task...");
  WiFi.disconnect(true, true);


    // Do two begins here with a delay to get around the problem that after upload 
    // the first wifi begin often fails.
    // From here https://www.esp32.com/viewtopic.php?t=12720
   WiFi.begin(ssid, ssidPass);
   delay(100);
   WiFi.begin(ssid, ssidPass);

    //check wi-fi is connected to wi-fi network
    while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");  
  Serial.println(WiFi.localIP());

  ydtoN2kUDP.begin(4444);

  // Now loop forever reading the YD data if available
  for(;;) {
    tN2kMsg msg;
    ydtoN2kUDP.readYD(msg);

    switch(msg.PGN) {
        case 127508: {
        unsigned char instance = 0xff;
        double voltage = 0.0;
        double current = 0.0;
        double temp = 273.0;
        unsigned char SID = 0xff;
        bool s = ParseN2kPGN127508(msg, instance, voltage, current, temp, SID);

        voltage += random(100) / 10.0;
        current -= random(100) / 10.0;
      
        Serial.printf("Instance %d PGN %d V %f I %f temp %f SID %d\n", instance, msg.PGN, voltage, current, temp, SID);

        switch(instance) {
          case 0:
          digital[0].value = voltage;
          digital[1].value = current;
          break;
          case 1:
          digital[2].value = voltage;
          digital[3].value = current;
          break;
        }
        }
        break;

        case 127513:
          // Not interested
          break;

        default: 
          // Catch any messages we don't expect
          // Serial.printf("PGN------%d\n", msg.PGN);
        break;

      }
    const TickType_t xDelay = 500 / portTICK_PERIOD_MS;
    vTaskDelay(xDelay);
  }
}



// Initilise the tasks
void setup() {
    // set up serial debug
    Serial.begin(115200);
 
    xTaskCreate(
      metersTask ,          // Function that should be called
      "Meters",         // Name of the task (for debugging)
      96000,            // Stack size (bytes)
      NULL,             // Parameter to pass
      3,                // Task priority
      NULL              // Task handle
    );

    // Web server
    
    /*
    xTaskCreate(
      webServerTask,
      "webserver",
      16000,
      NULL,
      2,
      NULL
      );
      */

      // WiFi manager
      xTaskCreate(
        wifiManagerTask,
        "wifi manager",
        32000,
        NULL,
        1, 
        NULL
        );
}

void loop(void) {}
