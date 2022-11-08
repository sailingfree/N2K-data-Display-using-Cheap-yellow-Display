/*

 Replacement instrument for a Simrad IS15 combi display.

 Uses an ESP32 and a TFT display to completely replace the inards of the IS15.
 The old IS15 stopped working completely and Simrad would not repair it or provide
 any details of the board for it to be repaired. 

 Needs Font 2 (also Font 4 if using large centered scale label)

  #########################################################################
  ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
  #########################################################################
 */

#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include "Free_Fonts.h" // Include the header file attached to this sketch


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
} Digital;

Digital digital[] = {
  {1.0, X0, Y0, 'i'},
  {2.0, X1, Y1, 'i'},
  {3.0, X2, Y2, 'f'},
  {4.0, X3, Y3, 'f'},
};

uint32_t updateTime = 0;       // time for next update

String label[6] = {"idx0","idx1","idx2","idx3"};

int d = 0;

 int xpos =  0;
  int ypos = 40;
void fontTest() {
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(xpos, ypos);    // Set cursor near top left corner of screen

  tft.setTextFont(GLCD);     // Select the orginal small GLCD font by using NULL or GLCD
  tft.println();             // Move cursor down a line
  tft.print("Original GLCD font");    // Print the font name onto the TFT screen
  tft.println();
  tft.println();

  tft.setFreeFont(FSB9);   // Select Free Serif 9 point font, could use:
  // tft.setFreeFont(&FreeSerif9pt7b);
  tft.println();          // Free fonts plot with the baseline (imaginary line the letter A would sit on)
  // as the datum, so we must move the cursor down a line from the 0,0 position
  tft.print("Serif Bold 9pt");  // Print the font name onto the TFT screen

  tft.setFreeFont(FSB12);       // Select Free Serif 12 point font
  tft.println();                // Move cursor down a line
  tft.print("Serif Bold 12pt"); // Print the font name onto the TFT screen

  tft.setFreeFont(FSB18);       // Select Free Serif 12 point font
  tft.println();                // Move cursor down a line
  tft.print("Serif Bold 18pt"); // Print the font name onto the TFT screen

  tft.setFreeFont(FSS24);       // Select Free Serif 24 point font
  tft.println();                // Move cursor down a line
  tft.print("Sans Serif Bold 24pt"); // Print the font name onto the TFT screen
}

// Main task for plotting the meters using data previously read
// Draw the display elements then loop forever
void metersTask(void * param) {
  Serial.println("Starting the meter draw task...");
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  //fontTest();
  //while(1) {
  //  sleep(1);
  //}
  
  digitalMeter(IDX0);       // Draw digitalue meter
  digitalMeter(IDX1);     // Draw digitalue meter
  digitalMeter(IDX2);     // Draw digitalue meter
  digitalMeter(IDX3);   // Draw digitalue meter

  updateTime = millis(); // Next update time
  Serial.println("Done setting up meters...");

  for(;;) {
    const TickType_t xDelay = 100 / portTICK_PERIOD_MS;
    vTaskDelay(xDelay);
    runMeters();
  }
}



void runMeters() {
  if (updateTime <= millis()) {
    updateTime = millis() + 1000; // Delay to limit speed of update
 
    updateDigital(IDX0);
    updateDigital(IDX1); 
    updateDigital(IDX2); 
    updateDigital(IDX3); 
  }
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

  switch(vtype) {
    case 'f':
    //dtostrf(value/1.0, 3, 1, buf);
    value += 0.2345;
    snprintf(buf, 5, "%3.1f", value);
    Serial.printf("Value = %f %3.1f\n", value, value);
    break;

    case 'i':
    default:
      snprintf(buf, 5, "%d", (int)value);   // we can fit 4 digits per cell.
      break;
  }

  uint16_t mainHeight = 8;

  // Blank the area where the new text is written using the font height
  uint32_t height = tft.fontHeight(mainHeight);
 
  tft.fillRect(xorig + X_MARGIN, yorig+height, 230, height, TFT_GREEN);
 
  // Update 

  tft.drawCentreString(buf, xorig+TFT_HEIGHT / 4, yorig + height, mainHeight);
//   tft.setCursor(xorig + TFT_HEIGHT/4, yorig + 50);
// tft.setFreeFont(FSS24);       // Select Free Serif 24 point font
//  tft.print(buf); // Print the font name onto the TFT screen

 // Draw the label one time only
 if(!digital[idx].doneLabel) {
  tft.drawCentreString(label[idx], xorig + TFT_HEIGHT / 4, yorig + 20, 4);
  digital[idx].doneLabel = true;
 }
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

// read from the signal k server
static void readSk()
{
  const char * host = "192.168.4.1";
  
  printf("Reading the sk server\n");
     // Use WiFiClient class to create TCP connections
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect(host, httpPort)) {
        Serial.println("connection failed");
        return;
    }

    // We now create a URI for the request
    String url = "http://192.168.1.102/signalk/v1/api/vessels/dragonfly/";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // Set up a POST request with the username and password for the signalk server
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String body = "{\"username\": \"admin\", \"password\": \"admin\"}";

  int httpResponseCode = http.POST(body);
  Serial.print("Response is ");
  Serial.println(httpResponseCode);

    /*
    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            Serial.println(">>> Client Timeout !");
            client.stop();
            return;
        }
    }

  String  data = "";
    // Read all the lines of the reply from server and print them to Serial
    while(client.available()) {
        String line = client.readStringUntil('\r');
        Serial.println(line);
        data = line;

    }
    Serial.println("Creating object");
        StaticJsonDocument <16000> doc;
        Serial.println("Created");
        DeserializationError error = deserializeJson(doc, data);
        // Test if parsing succeeds.
        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
            return;
        }
        Serial.println("Parsed");
        double ground_speed = doc["navigation"]["speedOverGround"]["value"];
        double water_speed  = doc["navigation"]["speedThroughWater"]["value"];
        double depth        = doc["environment"]["depth"]["belowTransducer"]["value"];
        double heading      = doc["navigation"]["headingMagnetic"]["value"];
        double wind         = doc["environment"]["wind"]["speedApparent"]["value"];
        Serial.println(ground_speed * 1.94, 2);  
        Serial.println(water_speed * 1.94, 2);
        Serial.println(depth, 2);

        Serial.print("HDG:");
        Serial.println(heading * 52.2957, 0);
        value[0] = ground_speed * 1.94;  // m/s to knots * 10
        vtype[0]='f';
        label[0] = "SOG Kts";
        value[1] = water_speed * 1.94;   
        vtype[1] = 'f';
        label[1] = "Boat Kts";
        value[2] = depth;
        vtype[2] = 'f';
        label[2] = "Depth m";
        double h = heading;
        h*= 57.2957;     // to degrees
        h = h - 180;       // adjust because the box points backwards
        if(h < 0 ) h +=360;
        if(h > 360) h =- 360;
        value[3] = h;
        vtype[3] = 'i';
        label[3] = "HDG mag";
 //   Serial.println("closing connection");
 */
    runMeters();
    sleep(1);
}

// manage the  wifi

const String hostname  = "damselfly1";    //Hostname for network discovery
const char * ssid      = "Dragonfly";     //SSID to connect to
const char * ssidPass  = "Hanse301";  // Password for wifi

// WiFi task.
void wifiManagerTask(void * param) 
{
  Serial.println("Starting WiFi manager task...");
  /*
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
*/
  for(;;) {
    //runMeters();
    //readSk();
    sleep(1);
    digital[0].value++;
    digital[1].value+=2;
    digital[2].value = rand() / 1.234;
    digital[3].value+=4;
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
      1,                // Task priority
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
        3, 
        NULL
        );
}

void loop(void) {}
