#include "config_input.h"
#include <memory.h>
#include <stdint.h>
#include "config.h"
#include "esp_system.h"
#ifdef TTGO_T1
#include "driver/gpio.h"    
#endif
void config_input_init(void) {
#ifdef TTGO_T1
    gpio_config_t gpio_cfg;
    memset(&gpio_cfg,0,sizeof(gpio_cfg));
    gpio_cfg.mode = GPIO_MODE_INPUT;
    gpio_cfg.pin_bit_mask = ((uint64_t)1) | (((uint64_t)1)<<35);
    gpio_cfg.pull_up_en = 1;
    ESP_ERROR_CHECK(gpio_config(&gpio_cfg));
#endif
}
void config_input_update(void) {
#ifdef TTGO_T1
    if(!gpio_get_level((gpio_num_t)0)||!gpio_get_level((gpio_num_t)35)) {
        config_add_value("configure","");
        esp_restart();
    }
#endif
}
