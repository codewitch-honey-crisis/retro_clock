#include "config_input.h"
#include "panel.h"
#include <memory.h>
#include <stdint.h>
#include "config.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void config_input_update(void) {
#ifdef BUTTON
    if(panel_button_read_all()) {    
        config_add_value("configure","");
        esp_restart();
    }
#endif
#ifdef TOUCH_BUS
    panel_touch_update();
    uint16_t x,y,s;
    size_t count = 1;
    panel_touch_read_raw(&count,&x,&y,&s);
    if(count) {
       
        config_add_value("configure","");
        esp_restart();
    }

#endif
}
