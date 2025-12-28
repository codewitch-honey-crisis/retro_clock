#include "display.hpp"
#include <memory.h>
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#ifdef LCD_SPI_HOST
#include "driver/spi_master.h"
#endif

uix::display lcd;

// UIX calls this to send bitmaps to the display
static void uix_flush(const gfx::rect16& bounds, const void *bitmap, void *state)
{
    // printf("FLUSH: (%d, %d)-(%d, %d) %dx%d\n",bounds.x1,bounds.y1,bounds.x2,bounds.y2,bounds.x2-bounds.x1+1,bounds.y2-bounds.y1+1);
    panel_lcd_flush(bounds.x1, bounds.y1, bounds.x2 , bounds.y2 , (void*)bitmap);
}
void panel_lcd_flush_complete() {
    lcd.flush_complete();
}
void display_init(void) {
#ifdef POWER
    panel_power_init();
#endif    
    panel_lcd_init();
#ifdef TOUCH_BUS
    panel_touch_init();
#endif    
#ifdef BUTTON
    panel_button_init();
#endif    

    lcd.buffer_size(LCD_TRANSFER_SIZE);
    lcd.buffer1((uint8_t*)panel_lcd_transfer_buffer());
    // 2nd buffer is for DMA performance, such that we can write one buffer while the other is transferring
    lcd.buffer2((uint8_t*)panel_lcd_transfer_buffer2());
    lcd.on_flush_callback(uix_flush);
}