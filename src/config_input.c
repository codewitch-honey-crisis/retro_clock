#include "config_input.h"
#include "panel.h"
#include <memory.h>
#include <stdint.h>
#include "config.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#ifdef BUTTON
static TickType_t pressed_ts = 0;
#endif
#ifdef TOUCH_BUS
static TickType_t touched_ts = 0;
#endif
void config_input_update(void) {
#ifdef BUTTON
    if(!panel_button_read_all()) {
        pressed_ts = xTaskGetTickCount();
    } else {
        if(xTaskGetTickCount()>pressed_ts+pdMS_TO_TICKS(250)) {
            config_add_value("configure","");
            esp_restart();
        }
    }
#endif
#ifdef TOUCH_BUS
    panel_touch_update();
    uint16_t x,y,s;
    size_t count = 1;
    panel_touch_read_raw(&count,&x,&y,&s);
    if(!count) {
        touched_ts = xTaskGetTickCount();
    } else {
        if(xTaskGetTickCount()>touched_ts+pdMS_TO_TICKS(250)) {
            config_add_value("configure","");
            esp_restart();
        }
    }
#endif
}
