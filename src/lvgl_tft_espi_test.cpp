#include <lvgl.h>
#include <TFT_eSPI.h>
#include <rotary_encoder.h>

TFT_eSPI tft = TFT_eSPI(); /* TFT instance */
static lv_disp_buf_t disp_buf;
static lv_color_t buf[320 * 10];
lv_obj_t * gauge;

#if USE_LV_LOG != 0
/* Serial debugging */
void my_print(lv_log_level_t level, const char * file, uint32_t line, const char * dsc)
{

    Serial.printf("%s@%d->%s\r\n", file, line, dsc);
    Serial.flush();
}
#endif

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
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
bool read_encoder(lv_indev_drv_t * indev, lv_indev_data_t * data)
{
    static int32_t last_val = 0;

    int32_t rval = rotaryEncoder.readEncoder();
    data->enc_diff = rval - last_val;
    last_val = rval;
    data->state = rotaryEncoder.isEncoderButtonDown() ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL; /* Dummy - no press */

    if(data->enc_diff > 0) {
        data->key=LV_KEY_UP;
        } else if(data->enc_diff < 0)  {
        data->key = LV_KEY_DOWN;
    }
    return false;    // Never any more data epected form this device
}

static void my_event_cb(lv_obj_t * obj, lv_event_t event)
{
    Serial.printf("EV %d\n", event);
}

void setup()
{
    Serial.begin(115200); /* prepare for possible serial debug */
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

    /*Initialize the input device driver*/
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_ENCODER;
    indev_drv.read_cb = read_encoder;
    lv_indev_t * rotary = lv_indev_drv_register(&indev_drv);

    // Create the group
    lv_group_t * g = lv_group_create();

    /* Create simple label */
    lv_obj_t *label = lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(label, "Hello Arduino! (V7.0.X)");
    lv_obj_align(label, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);

    // Create a gauge
    gauge = lv_gauge_create(lv_scr_act(), NULL);
    lv_gauge_set_value(gauge,0, 0);
    lv_obj_align(gauge, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 10);
    lv_obj_set_event_cb(gauge, my_event_cb);

    // Create a slider
    lv_obj_t * slider = lv_slider_create(lv_scr_act(), NULL);
    lv_obj_align(slider, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);

    // Spin box
    lv_obj_t *spinbox = lv_spinbox_create(lv_scr_act(), NULL);
    lv_obj_align(spinbox, NULL, LV_ALIGN_IN_TOP_RIGHT, 0, 0);
    lv_spinbox_set_range(spinbox, 0, 1000);

      // Add the gauge to the group
    lv_group_add_obj(g, gauge);
    lv_group_add_obj(g, slider);
    lv_group_add_obj(g, label);
    lv_group_add_obj(g, spinbox);

    lv_group_focus_obj(spinbox);

    // And the rotary encoder
    lv_indev_set_group(rotary, g);

    // Set to edit mode ready for first event
    lv_group_set_editing(g, true);
}


void loop()
{

    lv_task_handler(); /* let the GUI do its work */
    delay(5);
}
