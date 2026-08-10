#include "display.hpp"
#include <memory.h>
#include "esp_check.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#ifdef LCD_SPI_HOST
#include "driver/spi_master.h"
#endif
#if LCD_BUS == PANEL_BUS_MIPI || LCD_BUS == PANEL_BUS_RGB
#include "esp_cache.h"
#include "esp_memory_utils.h"

#define USE_DIRECT_MODE

// How many framebuffers the driver allocated. 1 == the FB is live/scanned out.
#if !defined(LCD_FRAMEBUFFER_COUNT)
#define ESPMON_FB_COUNT 1
#else
#define ESPMON_FB_COUNT LCD_FRAMEBUFFER_COUNT
#endif

// Push dirty cache lines for rows [y1..y2] out to PSRAM so the LCD DMA sees
// them. C2M (writeback) only - nothing but the CPU ever writes the FB, so no
// invalidate is ever needed. No-op on targets whose PSRAM cache is write-through.
static inline void uix_fb_writeback(const void* fb, int y1, int y2) {
#if CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32P4
    if (esp_ptr_external_ram(fb)) {
        const size_t stride = (size_t)LCD_WIDTH * 2;   // RGB565
        void*  start = (void*)(((uint8_t*)fb) + (size_t)y1 * stride);
        size_t len   = (size_t)(y2 - y1 + 1) * stride;
        ESP_ERROR_CHECK(esp_cache_msync(start, len,
            ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED));
    }
#else
    (void)fb; (void)y1; (void)y2;
#endif
}
#endif

uix::display lcd;

static void uix_flush(const gfx::rect16& bounds, const void* bmp, void* state) {
#if LCD_BUS == PANEL_BUS_MIPI || LCD_BUS == PANEL_BUS_RGB

#if ESPMON_FB_COUNT == 1
    // Single framebuffer: it IS the scanout buffer. Once the writeback lands,
    // the pixels are on screen. Nothing to recycle, nothing to wait for, so
    // completion is synchronous.
    uix_fb_writeback(bmp, bounds.y1, bounds.y2);
    lcd.flush_complete();
#else
    // Multiple framebuffers: we rendered into a hidden buffer. Writeback first
    // (DMA must not read stale lines), then retarget scanout at it. The swap
    // lands at the next VSYNC, so completion is NOT signalled here - the
    // on_vsync handler calls app.transfer_complete() once the swap is live.
    uix_fb_writeback(bmp, bounds.y1, bounds.y2);
    panel_lcd_flush(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, (void*)bmp);
#endif

#else   // transfer-buffer buses (SPI / I80 / QSPI)

#if LCD_TRANSFER_SIZE > 0
    panel_lcd_flush(bounds.x1, bounds.y1, bounds.x2, bounds.y2, (void*)bmp);
#if LCD_SYNC_TRANSFER == 1
    lcd.flush_complete();    // async case completes via panel_lcd_flush_complete()
#endif
#else
#error "LCD_TRANSFER_SIZE == 0 requires an RGB or MIPI panel"
#endif

#endif
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

#ifndef USE_DIRECT_MODE
    lcd.buffer_size(LCD_TRANSFER_SIZE);
    lcd.buffer1((uint8_t*)panel_lcd_transfer_buffer());
    lcd.buffer2((uint8_t*)panel_lcd_transfer_buffer2());
#else
    lcd.update_mode(uix::screen_update_mode::direct);
    lcd.buffer_size((LCD_WIDTH*LCD_HEIGHT*LCD_BIT_DEPTH+7)/8);
    lcd.buffer1((uint8_t*)panel_lcd_framebuffer(0));
    lcd.buffer2((uint8_t*)panel_lcd_framebuffer(1));
#endif
    
    lcd.on_flush_callback(uix_flush);
}