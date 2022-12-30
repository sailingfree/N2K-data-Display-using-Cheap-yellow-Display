// Select which tft driver to use
// TFT_LVGL - use the lvgl library
// TFT_ESPI - Use the handcrafted tft_espi library

#define TFT_LVGL 1
#define TFT_ESPI 2

#define TFT_TYPE  TFT_LVGL

#if TFT_TYPE == TFT_LVGL
// Use the lvgl version

#elif TFT_TYPE == TFT_ESPI
// Use the handcrafted screen layout

#else
#error Please select TFT_LVGL or TFT_ESPI
#endif

typedef struct {
    double value;
    uint32_t x;
    uint32_t y;
    char type;
    String label;
    bool doneLabel;
    uint32_t oldx;
    uint32_t oldw;
    double oldvalue;
} Digital;

//void metersTask(void* param);
void metersSetup();
void metersWork();
void setMeter(uint16_t, double);


typedef enum {
    HOUSEV,
    HOUSEI,
    ENGINEV,
    RPM
} MeterIdx;
