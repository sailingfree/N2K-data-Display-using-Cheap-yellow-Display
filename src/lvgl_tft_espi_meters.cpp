#include <TFT_eSPI.h>
#include <lvgl.h>
#include <rotary_encoder.h>

TFT_eSPI tft = TFT_eSPI(); /* TFT instance */
static lv_disp_buf_t disp_buf;
static lv_color_t buf[320 * 10];
lv_obj_t *gauge;

#if USE_LV_LOG != 0
/* Serial debugging */
void my_print(lv_log_level_t level, const char *file, uint32_t line, const char *dsc) {
    Serial.printf("%s@%d->%s\r\n", file, line, dsc);
    Serial.flush();
}
#endif

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors(&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

/*
 * Read the input rotary encoder
 * Read the value and the button state
 */
bool read_encoder(lv_indev_drv_t *indev, lv_indev_data_t *data) {
    static int32_t last_val = 0;

    int32_t rval = rotaryEncoder.readEncoder();
    data->enc_diff = rval - last_val;
    last_val = rval;
    data->state = rotaryEncoder.isEncoderButtonDown() ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL; /* Dummy - no press */

    if (data->enc_diff > 0) {
        data->key = LV_KEY_UP;
    } else if (data->enc_diff < 0) {
        data->key = LV_KEY_DOWN;
    }
    return false;  // Never any more data epected from this device
}

/*Read the touchpad*/
/* from here https://github.com/lvgl/lvgl/blob/master/examples/arduino/LVGL_Arduino/LVGL_Arduino.ino
 */
bool my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    uint16_t touchX, touchY;

    bool touched = tft.getTouch(&touchX, &touchY, 600);

    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        data->state = LV_INDEV_STATE_PR;

        /*Set the coordinates*/
        data->point.x = touchX;
        data->point.y = touchY;

        Serial.print("Data x ");
        Serial.println(touchX);

        Serial.print("Data y ");
        Serial.println(touchY);
    }

    return false;
}

static void my_event_cb(lv_obj_t *obj, lv_event_t event) {
    Serial.printf("EV %d\n", event);
}

class Indicator {
   public:
    Indicator(lv_obj_t *parent, const char *label, uint32_t x, uint32_t y);
    void setValue(const char *value);

   //private:
    lv_obj_t *container;
    lv_obj_t *label;
    lv_obj_t *text;
};

Indicator::Indicator(lv_obj_t *parent, const char *name, uint32_t x, uint32_t y) {
    container = lv_cont_create(parent, NULL);
    lv_obj_set_style_local_border_width(container, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 2);
    lv_obj_set_style_local_border_color(container, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_cont_set_layout(container, LV_LAYOUT_CENTER);
    lv_cont_set_fit(container, LV_FIT_NONE);
    lv_obj_set_size(container, 480 / 2, 320 / 3);
    lv_obj_set_style_local_margin_left(container, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_margin_right(container, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);

    label = lv_label_create(container, NULL);
    lv_obj_set_style_local_text_font(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_18);
    lv_label_set_text(label, name);

    text = lv_label_create(container, NULL);
    lv_obj_set_style_local_text_font(text, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_48);
    lv_label_set_text(text, "---");
    text->event_cb = my_event_cb;

    lv_obj_set_pos(container, x, y);
}

void Indicator::setValue(const char *value) {
    lv_label_set_text(text, value);
}

// Define the screen
Indicator *ind[4];

lv_obj_t * vlabel;

void metersSetup() {
    rotary_setup();
    lv_init();
#if USE_LV_LOG != 0
    lv_log_register_print_cb(my_print); /* register print function for debugging */
#endif

    //   tft.begin(); /* TFT init */
    tft.init();
    tft.setRotation(1); /* Landscape orientation */

    lv_disp_buf_init(&disp_buf, buf, NULL, 320 * 10);

    /*Initialize the display*/
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 480;
    disp_drv.ver_res = 320;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    /*Initialize the rotary encoder input device drivers */
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_ENCODER;
    indev_drv.read_cb = read_encoder;
    lv_indev_t *rotary = lv_indev_drv_register(&indev_drv);

    /*Initialize the touch input device driver*/
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_t *touch = lv_indev_drv_register(&indev_drv);

    // Create the group for the rotary
    lv_group_t *g = lv_group_create();

    // And for the touch
    lv_group_t *t = lv_group_create();

    // Create a container
    lv_obj_t *cont;
    cont = lv_cont_create(lv_scr_act(), NULL);
    lv_obj_set_auto_realign(cont, true);                   /*Auto realign when the size changes*/
    lv_obj_align_origo(cont, NULL, LV_ALIGN_CENTER, 0, 0); /*This parametrs will be sued when realigned*/
    lv_cont_set_fit(cont, LV_FIT_PARENT);
    lv_cont_set_layout(cont, LV_LAYOUT_OFF);
    lv_obj_set_style_local_pad_left(cont, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_right(cont, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_top(cont, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_bottom(cont, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_inner(cont, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_margin_left(cont, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_margin_right(cont, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);

    /* Create simple label */
    // lv_obj_t *label = lv_label_create(cont, NULL);
    // lv_label_set_text(label, "Hello Arduino! (V7.0.X)");
    // lv_obj_align(label, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);

    //  lv_obj_t * m1 = lv_cont_create(cont, NULL);
    //  lv_obj_set_style_local_border_width(m1, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 2);
    //  lv_obj_set_style_local_border_color(m1, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    //  lv_cont_set_layout(m1, LV_LAYOUT_CENTER);
    //  lv_cont_set_fit(m1, LV_FIT_TIGHT);
    //  lv_obj_t * t1 = lv_label_create(m1, NULL);
    //  lv_obj_set_style_local_text_font(t1, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_18);
    //  lv_label_set_text(t1, "House Voltage");
    //  lv_obj_set_size(t1, 200,100);

    //  lv_obj_t * t2 = lv_label_create(m1, t1);
    //  lv_obj_set_style_local_text_font(t2, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_38);
    //  lv_label_set_text(t2,"12.62V");

    ind[0] = new Indicator(cont, "House Voltage", 0, 0);
    ind[1] = new Indicator(cont, "House Current",0, TFT_WIDTH / 3);
    ind[2] = new Indicator(cont, "Engine Voltage", 0, 2 * TFT_WIDTH / 3);
 
    // Create a container and a gauge
    //lv_cont_set_layout(cont, LV_LAYOUT_COLUMN_RIGHT);
    lv_obj_t * container = lv_cont_create(cont, NULL);
    lv_obj_set_style_local_border_width(container, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 2);
    lv_obj_set_style_local_border_color(container, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_cont_set_layout(container, LV_LAYOUT_COLUMN_MID);
    lv_cont_set_fit(container, LV_FIT_NONE);
    lv_obj_set_size(container, 480 / 2, (320 / 6 * 5));
    lv_obj_set_style_local_margin_left(container, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_margin_right(container, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_pos(container, TFT_HEIGHT/2, 0);

 

//    ind[3] = new Indicator(cont, "RPM", TFT_HEIGHT /2, 2 * TFT_WIDTH / 3);
/*
    label = lv_label_create(container, NULL);
    lv_obj_set_style_local_text_font(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_18);
    lv_label_set_text(label, name);

    text = lv_label_create(container, NULL);
    lv_obj_set_style_local_text_font(text, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_48);
    lv_label_set_text(text, "---");
    text->event_cb = my_event_cb;
    */
    uint8_t major = 8;
    uint8_t minor = (36);
    gauge = lv_gauge_create(container, NULL);
    lv_gauge_set_value(gauge,0, 0);
    lv_gauge_set_range(gauge, 0, 35);
    lv_gauge_set_scale(gauge, 270, minor, major);
    lv_gauge_set_critical_value(gauge, 30);
    lv_obj_set_style_local_text_font(gauge, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                        &lv_font_montserrat_16);
    lv_obj_align(gauge, container, LV_ALIGN_IN_TOP_MID, 0, 0);

   // And the Value label
    vlabel = lv_label_create(container, NULL);
    lv_label_set_text(vlabel, "0000");
    lv_obj_align(vlabel, container, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_local_text_font(vlabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                    &lv_font_montserrat_32);


//    lv_obj_align(gauge, NULL, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 0);

    // Create a slider
    // lv_obj_t * slider = lv_slider_create(cont, NULL);
    // lv_obj_align(slider, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);

    // Spin box
    //  lv_obj_t *spinbox = lv_spinbox_create(cont, NULL);
    //   lv_obj_align(spinbox, NULL, LV_ALIGN_IN_TOP_RIGHT, 0, 0);
    //   lv_spinbox_set_range(spinbox, 0, 1000);

    //  lv_group_add_obj(g, slider);
    //  lv_group_add_obj(g, spinbox);
    //  lv_group_add_obj(t, slider);

    //  lv_group_focus_obj(spinbox);

    // And the rotary encoder
    //  lv_indev_set_group(rotary, g);

    // touch device
    // lv_indev_set_group(touch, t);

    // Set to edit mode ready for first event
    // lv_group_set_editing(g, true);

    lv_refr_now(NULL);
}

// Update the meters. Called regularly from the main loop/task
void metersWork(void) {
    lv_task_handler(); /* let the GUI do its work */
}

// Set the value of a meter using a double
void setMeter(uint16_t idx, double value) {
    if(idx < 3) {
    String v(value, 2);
    ind[idx]->setValue(v.c_str());
    }
    if(idx == 3) {
        String lvalue(value, 0);
        lv_gauge_set_value(gauge, 0, value / 100);
        lv_label_set_text(vlabel, lvalue.c_str());
    }
}

// Set the value of a meter using a string
void setMeter(uint16_t idx, String & string) {
    ind[idx]->setValue(string.c_str());
}