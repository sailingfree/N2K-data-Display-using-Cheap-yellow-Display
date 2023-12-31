#include <lvgl.h>

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


// This class implements a rectangle comntainer which has a main display for 
// eg voltage, a smaller header. It is designed to work with the lvgl library
// on an ESP32 or similar. 
// It has a fixed size.
class Indicator {
   public:
    // Constructor: 
    Indicator(lv_obj_t *parent, const char *label, uint32_t x, uint32_t y);
    void setValue(const char *value);

   //private:
    lv_obj_t *container;
    lv_obj_t *label;
    lv_obj_t *text;
};


//void metersTask(void* param);
void metersSetup();
void metersWork();
void setMeter(int scr, int ind, double, char *);
void setGauge(int scr, double);
void setVlabel(int, String &);
void setilabel(int scr, String &);

// Define the screens
typedef enum {
    SCR_ENGINE,
    SCR_NAV,
    SCR_MAX
}Screens;

// Indicator indexes
typedef enum {
    HOUSEV      = 0,
    HOUSEI      = 1,
    ENGINEV     = 2,
    SOG         = 0,
    DEPTH       = 1,
    HDG         = 2,
} MeterIdx;
