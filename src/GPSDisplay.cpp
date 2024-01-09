#include <Arduino.h>
#include <tftscreen.h>
#include <lvgl.h>
#include <NMEA2000.h>
#include <GPSDisplay.h>

GPSDisplay::GPSDisplay(lv_obj_t * parentin, uint32_t xin, uint32_t yin){
    parent = parentin;
    x = xin;
    y = yin;
    w = TFT_HEIGHT / 2;
    h = TFT_HEIGHT / 2;


    // Constructor. Binds to the parent object.
    container = lv_cont_create(parent, NULL);
    lv_obj_set_style_local_border_width(container, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 2);
    lv_obj_set_style_local_border_color(container, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_cont_set_layout(container, LV_LAYOUT_CENTER);
    lv_cont_set_fit(container, LV_FIT_NONE);
    lv_obj_set_size(container, w, h);
    lv_obj_set_style_local_margin_left(container, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_margin_right(container, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);

    lv_obj_set_pos(container, xin, yin);

    clear();   // Draw the parts without any satellites
}

void GPSDisplay::clear() {
    lv_obj_t * arc = lv_arc_create(container, NULL);
    lv_obj_set_size(arc, w, h);
    lv_arc_set_bg_angles(arc, 0, 359);
    lv_arc_set_rotation(arc, 270);
}

void GPSDisplay::plot(tSatelliteInfo sat) {
    
}