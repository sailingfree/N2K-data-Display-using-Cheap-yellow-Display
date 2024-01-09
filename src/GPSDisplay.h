#include <lvgl.h>
#include <NMEA2000.h>
#include <N2kMessages.h>

class GPSDisplay {
    public:

    GPSDisplay(lv_obj_t * parent, uint32_t x, uint32_t y);

    void clear();
    void plot(tSatelliteInfo sat);

    private: 
    uint32_t x, y, w, h;
    lv_obj_t * parent;
    lv_obj_t * container;
};