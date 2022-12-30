// The tft espi LCD meters version of the display.

#include <ArduinoOTA.h>

// Define the quadrants
#define TFT_GREY 0x5AEB
#define IDX0 0
#define X0 0
#define Y0 0

#define IDX1 1
#define X1 TFT_HEIGHT / 2
#define Y1 0

#define IDX2 2
#define X2 0
#define Y2 TFT_WIDTH / 2

#define IDX3 3
#define X3 TFT_HEIGHT / 2
#define Y3 TFT_WIDTH / 2

// Margins
#define X_MARGIN 5
#define Y_MARGIN 2

