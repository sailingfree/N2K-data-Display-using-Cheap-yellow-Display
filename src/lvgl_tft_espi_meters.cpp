#include <TFT_eSPI.h>
#include <lvgl.h>
#include <rotary_encoder.h>
#include <tftscreen.h>
#include <GwPrefs.h>

// A library for interfacing with the touch screen
//
// Can be installed from the library manager (Search for "XPT2046")
//https://github.com/PaulStoffregen/XPT2046_Touchscreen
#include <XPT2046_Touchscreen.h>


// ----------------------------
// Touch Screen pins
// ----------------------------

// The CYD touch uses some non default
// SPI pins

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

// ----------------------------

SPIClass mySpi = SPIClass(HSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);


TFT_eSPI tft = TFT_eSPI(); /* TFT instance */
static lv_disp_buf_t disp_buf;
static lv_color_t buf[TFT_WIDTH * 10];


#if USE_LV_LOG != 0
/* Serial debugging */
void my_print(lv_log_level_t level, const char *file, uint32_t line, const char *dsc) {
    Serial.printf("%s@%d->%s\r\n", file, line, dsc);
    Serial.flush();
}
#endif

lv_obj_t * createEngineScreen(int screen);
lv_obj_t * createNavScreen(int screen );

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

// Define the screens and their data items
Indicator * ind[SCR_MAX][6];
lv_obj_t  * screen[SCR_MAX];
lv_obj_t  * gauges[SCR_MAX];
lv_obj_t  * vals[SCR_MAX];
lv_obj_t  * infos[SCR_MAX];


// Helper
#define MAX_TOUCH_X (4096 / TFT_HEIGHT)
#define MAX_TOUCH_Y (4096 / TFT_WIDTH)

void printTouchToSerial(TS_Point p) {
  Serial.print("Pressure = ");
  Serial.print(p.z);
  Serial.print(", x = ");
  Serial.print(p.x/MAX_TOUCH_X);
  Serial.print(", y = ");
  Serial.print(p.y/MAX_TOUCH_Y);
  Serial.println();
}

/*
* Read the touch panel
*/

bool read_touch(lv_indev_drv_t *indev, lv_indev_data_t *data) {
    if (ts.tirqTouched() && ts.touched()) {
        TS_Point p = ts.getPoint();
    //    printTouchToSerial(p);
        data->point.x = p.x / MAX_TOUCH_X;
        data->point.y = p.y / MAX_TOUCH_Y;
        data->state = LV_INDEV_STATE_PR or LV_INDEV_STATE_REL;
    }
    return false;
}

static int iiscrnum = 0;    // Screen number selected

// Handle the touch event.
static void my_event_cb(lv_obj_t *obj, lv_event_t event) {
 //   Serial.printf("EV %d\n", event);
    if(event == 7) {
        // Key up swap screens
        iiscrnum = (iiscrnum + 1) % SCR_MAX ;
        lv_scr_load(screen[iiscrnum]);
        String val(iiscrnum);
        GwSetVal(GWSCREEN, val);
    }
}



// Constructor. Binds to the parent object.
Indicator::Indicator(lv_obj_t *parent, const char *name, uint32_t x, uint32_t y) {
    container = lv_cont_create(parent, NULL);
    lv_obj_set_style_local_border_width(container, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 2);
    lv_obj_set_style_local_border_color(container, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_cont_set_layout(container, LV_LAYOUT_CENTER);
    lv_cont_set_fit(container, LV_FIT_NONE);
    lv_obj_set_size(container, TFT_HEIGHT / 2, TFT_WIDTH / 3);
    lv_obj_set_style_local_margin_left(container, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_margin_right(container, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);

    label = lv_label_create(container, NULL);
    lv_obj_set_style_local_text_font(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_18);
    lv_label_set_text(label, name);

    text = lv_label_create(container, NULL);
    lv_obj_set_style_local_text_font(text, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_32);
    lv_label_set_text(text, "---");
    text->event_cb = my_event_cb;

    lv_obj_set_pos(container, x, y);
}

void Indicator::setValue(const char *value) {
    lv_label_set_text(text, value);
}



void touch_init() {
    // Start the SPI for the touch screen and init the TS library
    mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    ts.begin(mySpi);
    ts.setRotation(1);

}

void metersSetup() {
    rotary_setup();
    lv_init();
#if USE_LV_LOG != 0
    lv_log_register_print_cb(my_print); /* register print function for debugging */
#endif
    touch_init();

    // Get the last screen number if set and use that
    String scrnum = GwGetVal(GWSCREEN);
    if(scrnum != "---") {
        iiscrnum = scrnum.toInt() % SCR_MAX;
    }

    //   tft.begin(); /* TFT init */
    tft.init();
    tft.fillRect(0, 0, TFT_WIDTH, TFT_HEIGHT, 0);
    tft.setRotation(1); /* Landscape orientation */

    lv_disp_buf_init(&disp_buf, buf, NULL, TFT_WIDTH * 10);

    /*Initialize the display*/
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = TFT_HEIGHT;
    disp_drv.ver_res = TFT_WIDTH;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    /*Initialize the rotary encoder input device drivers */
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_ENCODER;
    indev_drv.read_cb = read_encoder;
    lv_indev_t *rotary = lv_indev_drv_register(&indev_drv);

    // Create the group for the rotary
    lv_group_t *g = lv_group_create();

    // And for the touch
    lv_indev_drv_t touch_dev;
    lv_indev_drv_init(&touch_dev);
    touch_dev.type = LV_INDEV_TYPE_POINTER;
    touch_dev.read_cb = read_touch;
    lv_indev_t *touch = lv_indev_drv_register(&touch_dev);

    lv_group_t *t = lv_group_create();

    // Create a container for the engine screen
    screen[SCR_ENGINE] = createEngineScreen(SCR_ENGINE);
    screen[SCR_NAV] = createNavScreen(SCR_NAV);

    // Load the first screen
    lv_scr_load(screen[iiscrnum]);
}

lv_obj_t * createEngineScreen(int scr) {
    lv_obj_t * gauge;
    lv_obj_t * vlabel, *ilabel;
    lv_obj_t * screen = lv_obj_create(NULL, NULL);
    //screen = lv_scr_act();
    lv_obj_t * cont = lv_cont_create(screen, NULL);
    lv_obj_set_auto_realign(cont, true);                   /*Auto realign when the size changes*/
    lv_obj_align_origo(cont, NULL, LV_ALIGN_CENTER, 0, 0); /*This parametrs will be used when re-aligned*/
    lv_cont_set_fit(cont, LV_FIT_PARENT);
    lv_cont_set_layout(cont, LV_LAYOUT_OFF);
    lv_obj_set_style_local_pad_left(cont, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_right(cont, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_top(cont, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_bottom(cont, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_inner(cont, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_margin_left(cont, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_margin_right(cont, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);

    ind[scr][0] = new Indicator(cont, "House Voltage", 0, 0);
    ind[scr][1] = new Indicator(cont, "House Current",0, TFT_WIDTH / 3);
    ind[scr][2] = new Indicator(cont, "Engine Voltage", 0, 2 * TFT_WIDTH / 3);
 
    // Create a container and a gauge
    //lv_cont_set_layout(cont, LV_LAYOUT_COLUMN_RIGHT);
    lv_obj_t * container = lv_cont_create(cont, NULL);
    lv_obj_set_style_local_border_width(container, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 2);
    lv_obj_set_style_local_border_color(container, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_cont_set_layout(container, LV_LAYOUT_COLUMN_MID);
    lv_cont_set_fit(container, LV_FIT_NONE);
    lv_obj_set_size(container, TFT_HEIGHT / 2, (TFT_WIDTH / 6 * 5));
    lv_obj_set_style_local_margin_left(container, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_margin_right(container, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_pos(container, TFT_HEIGHT/2, 0);
    lv_obj_set_style_local_pad_inner(container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 2);


    // Gauge for the RPM
    uint8_t major = 8;              // Major ticks - 500RPM each
    uint8_t minor = (36);           // max is 3500 RPM
    gauge = lv_gauge_create(container, NULL);
    lv_obj_set_size(gauge, TFT_HEIGHT/2, TFT_HEIGHT/2);
    lv_obj_set_style_local_pad_inner(gauge, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 10);
    lv_obj_set_style_local_pad_left(gauge, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 2);
    lv_obj_set_style_local_pad_right(gauge, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 2);
    lv_obj_set_style_local_pad_top(gauge, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 2);
    lv_obj_set_style_local_pad_bottom(gauge, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 2);

    lv_gauge_set_value(gauge,0, 0);
    lv_gauge_set_range(gauge, 0, 35);
    lv_gauge_set_scale(gauge, 270, minor, major);
    lv_gauge_set_critical_value(gauge, 30);
    lv_obj_set_style_local_text_font(gauge, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                        &lv_font_montserrat_16);
    lv_obj_align(gauge, container, LV_ALIGN_IN_TOP_MID, 0, 0);
    gauges[scr] = gauge;

   // And the Value label
    vlabel = lv_label_create(container, NULL);
    lv_label_set_text(vlabel, "0000");
    lv_obj_align(vlabel, container, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_local_text_font(vlabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                    &lv_font_montserrat_24);

    vals[scr] = vlabel;
    // Info at the bottom
    ilabel = lv_label_create(cont, NULL);
    lv_label_set_text(ilabel, "000.000.000.000");
    lv_obj_align(ilabel, cont, LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_local_text_font(ilabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                    &lv_font_montserrat_12);
    infos[scr] = ilabel;
    
    // Event callback
    lv_obj_set_event_cb(gauge, my_event_cb);   /*Assign an event callback*/
    lv_obj_set_event_cb(ind[scr][0]->container, my_event_cb);   /*Assign an event callback*/
    lv_obj_set_event_cb(ind[scr][1]->container, my_event_cb);   /*Assign an event callback*/
    lv_obj_set_event_cb(ind[scr][2]->container, my_event_cb);   /*Assign an event callback*/

    return screen;
}

lv_obj_t * createNavScreen(int scr) {
    lv_obj_t *gauge;
    lv_obj_t * vlabel, *ilabel;
    lv_obj_t * screen = lv_obj_create(NULL, NULL);
    lv_obj_t * cont = lv_cont_create(screen, NULL);
    lv_obj_set_auto_realign(cont, true);                   /*Auto realign when the size changes*/
    lv_obj_align_origo(cont, NULL, LV_ALIGN_CENTER, 0, 0); /*This parametrs will be used when re-aligned*/
    lv_cont_set_fit(cont, LV_FIT_PARENT);
    lv_cont_set_layout(cont, LV_LAYOUT_OFF);
    lv_obj_set_style_local_pad_left(cont, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_right(cont, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_top(cont, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_bottom(cont, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_inner(cont, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_margin_left(cont, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_margin_right(cont, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);



    ind[scr][0] = new Indicator(cont, "SOG", 0, 0);
    ind[scr][1] = new Indicator(cont, "Depth",0, TFT_WIDTH / 3);
    ind[scr][2] = new Indicator(cont, "HDG", 0, 2 * TFT_WIDTH / 3);
 
    // Create a container and a gauge
    //lv_cont_set_layout(cont, LV_LAYOUT_COLUMN_RIGHT);
    lv_obj_t * container = lv_cont_create(cont, NULL);
    lv_obj_set_style_local_border_width(container, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 2);
    lv_obj_set_style_local_border_color(container, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_cont_set_layout(container, LV_LAYOUT_COLUMN_MID);
    lv_cont_set_fit(container, LV_FIT_NONE);
    lv_obj_set_size(container, TFT_HEIGHT / 2, (TFT_WIDTH / 6 * 5));
    lv_obj_set_style_local_margin_left(container, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_margin_right(container, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_pos(container, TFT_HEIGHT/2, 0);
    lv_obj_set_style_local_pad_inner(container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 2);


    // Gauge for the Wind
    uint8_t label_cnt = 5;              // Major ticks N S E W
    uint8_t sub_div  = 8;
    uint8_t line_cnt = line_cnt = (sub_div + 1) * (label_cnt - 1) + 1;           // Max
   
    static lv_style_t style;
    lv_style_init(&style);
    gauge = lv_gauge_create(container, NULL);
    lv_obj_set_size(gauge, TFT_HEIGHT/2, TFT_HEIGHT/2);
    lv_obj_set_style_local_pad_inner(gauge, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 10);
    lv_obj_set_style_local_pad_left(gauge, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 2);
    lv_obj_set_style_local_pad_right(gauge, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 2);
    lv_obj_set_style_local_pad_top(gauge, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 2);
    lv_obj_set_style_local_pad_bottom(gauge, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 2);

    lv_gauge_set_value(gauge,0, 0);
    lv_gauge_set_range(gauge, 1, 359);
    lv_gauge_set_scale(gauge, 360, line_cnt, label_cnt);
    lv_gauge_set_angle_offset(gauge, 181);
    lv_gauge_set_critical_value(gauge, 0);
    lv_style_set_scale_end_color(&style, LV_STATE_DEFAULT, LV_COLOR_BLUE);
    lv_style_set_line_color(&style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_obj_set_style_local_text_font(gauge, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                        &lv_font_montserrat_16);
    lv_obj_add_style(gauge, LV_GAUGE_PART_MAIN, &style);
    lv_obj_align(gauge, container, LV_ALIGN_IN_TOP_MID, 0, 0);
    gauges[scr] = gauge;

   // And the Value label
    vlabel = lv_label_create(container, NULL);
    lv_label_set_text(vlabel, "000");
    lv_obj_align(vlabel, container, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_local_text_font(vlabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                    &lv_font_montserrat_24);
    vals[scr] = vlabel;

    // Info at the bottom
    ilabel = lv_label_create(cont, NULL);
    lv_label_set_text(ilabel, "000.000.000.000");
    lv_obj_align(ilabel, cont, LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_local_text_font(ilabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                    &lv_font_montserrat_12);
    infos[scr] = ilabel;

    // Event callback
    lv_obj_set_event_cb(gauge, my_event_cb);   /*Assign an event callback*/
    lv_obj_set_event_cb(ind[scr][0]->container, my_event_cb);   /*Assign an event callback*/
    lv_obj_set_event_cb(ind[scr][1]->container, my_event_cb);   /*Assign an event callback*/
    lv_obj_set_event_cb(ind[scr][2]->container, my_event_cb);   /*Assign an event callback*/

    return screen;
}


// Update the meters. Called regularly from the main loop/task
void metersWork(void) {
    lv_task_handler(); /* let the GUI do its work */
}

// Set the value of a meter using a double
void setMeter(int scr, int idx, double value, char * units) {
//    Serial.printf("SET %d %d %f\n", scr, idx, value);
    if(idx < 3) {
        String v(value, 2);
        v += units;
        ind[scr][idx]->setValue(v.c_str());
    }
}

void setGauge(int scr, double value) {
        String lvalue(value, 0);
        lv_gauge_set_value(gauges[scr], 0, value);
}

// Set the value of a meter using a string
void setMeter(int scr, int idx, String & string) {
    ind[scr][idx]->setValue(string.c_str());
}


void setVlabel(int scr, String & str) {
         lv_label_set_text(vals[scr], str.c_str());
}

void setilabel(int scr, String & str) {
    lv_label_set_text(infos[scr], str.c_str());
}