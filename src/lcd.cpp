#include "lcd.hpp"
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
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)state;
    // pass the draw buffer to the driver
    esp_lcd_panel_draw_bitmap(panel_handle, bounds.x1, bounds.y1, bounds.x2 + 1, bounds.y2 + 1, bitmap);
}
#ifdef LCD_PIN_NUM_VSYNC
// LCD Panel API calls this
bool lcd_flush_complete(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx) {
    // let the display know the flush has finished
    lcd.flush_complete();
    return true;
}

static volatile bool lcd_is_vsync = 0;
// LCD Panel API calls this
bool lcd_vsync(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx) {
    lcd_is_vsync=1;
    return true;
}
#endif
#ifdef LCD_SPI_HOST
static IRAM_ATTR bool lcd_on_flush_complete(esp_lcd_panel_io_handle_t lcd_io, esp_lcd_panel_io_event_data_t* edata, void* user_ctx) {
    // let the display know the flush has finished
    lcd.flush_complete();
    return true;
}
#endif
void lcd_init(void) {
#if LCD_PIN_NUM_BCKL >= 0
    gpio_config_t bk_gpio_config;
    memset(&bk_gpio_config,0,sizeof(gpio_config_t));
    bk_gpio_config.mode = GPIO_MODE_OUTPUT;
    bk_gpio_config.pin_bit_mask = 1ULL << LCD_PIN_NUM_BCKL;
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    gpio_set_level((gpio_num_t)LCD_PIN_NUM_BCKL, LCD_BCKL_OFF_LEVEL);
#endif
    
    esp_lcd_panel_handle_t panel_handle = NULL;
#ifdef LCD_PIN_NUM_VSYNC
    esp_lcd_rgb_panel_config_t panel_config;
    memset(&panel_config,0,sizeof(esp_lcd_rgb_panel_config_t));
    panel_config.data_width = 16; // RGB565 in parallel mode, thus 16bit in width
    //panel_config.dma_burst_size = 64;
    panel_config.num_fbs = 1,
    panel_config.clk_src = LCD_CLK_SRC_DEFAULT,
    panel_config.disp_gpio_num = -1,
    panel_config.pclk_gpio_num = LCD_PIN_NUM_CLK,
    panel_config.vsync_gpio_num = LCD_PIN_NUM_VSYNC,
    panel_config.hsync_gpio_num = LCD_PIN_NUM_HSYNC,
    panel_config.de_gpio_num = LCD_PIN_NUM_DE,
#if !defined(LCD_SWAP_COLOR_BYTES) || LCD_SWAP_COLOR_BYTES == false
    panel_config.data_gpio_nums[0]=LCD_PIN_NUM_D00;
    panel_config.data_gpio_nums[1]=LCD_PIN_NUM_D01;
    panel_config.data_gpio_nums[2]=LCD_PIN_NUM_D02;
    panel_config.data_gpio_nums[3]=LCD_PIN_NUM_D03;
    panel_config.data_gpio_nums[4]=LCD_PIN_NUM_D04;
    panel_config.data_gpio_nums[5]=LCD_PIN_NUM_D05;
    panel_config.data_gpio_nums[6]=LCD_PIN_NUM_D06;
    panel_config.data_gpio_nums[7]=LCD_PIN_NUM_D07;
    panel_config.data_gpio_nums[8]=LCD_PIN_NUM_D08;
    panel_config.data_gpio_nums[9]=LCD_PIN_NUM_D09;
    panel_config.data_gpio_nums[10]=LCD_PIN_NUM_D10;
    panel_config.data_gpio_nums[11]=LCD_PIN_NUM_D11;
    panel_config.data_gpio_nums[12]=LCD_PIN_NUM_D12;
    panel_config.data_gpio_nums[13]=LCD_PIN_NUM_D13;
    panel_config.data_gpio_nums[14]=LCD_PIN_NUM_D14;
    panel_config.data_gpio_nums[15]=LCD_PIN_NUM_D15;
#else
    panel_config.data_gpio_nums[0]=LCD_PIN_NUM_D08;
    panel_config.data_gpio_nums[1]=LCD_PIN_NUM_D09;
    panel_config.data_gpio_nums[2]=LCD_PIN_NUM_D10;
    panel_config.data_gpio_nums[3]=LCD_PIN_NUM_D11;
    panel_config.data_gpio_nums[4]=LCD_PIN_NUM_D12;
    panel_config.data_gpio_nums[5]=LCD_PIN_NUM_D13;
    panel_config.data_gpio_nums[6]=LCD_PIN_NUM_D14;
    panel_config.data_gpio_nums[7]=LCD_PIN_NUM_D15;
    panel_config.data_gpio_nums[8]=LCD_PIN_NUM_D00;
    panel_config.data_gpio_nums[9]=LCD_PIN_NUM_D01;
    panel_config.data_gpio_nums[10]=LCD_PIN_NUM_D02;
    panel_config.data_gpio_nums[11]=LCD_PIN_NUM_D03;
    panel_config.data_gpio_nums[12]=LCD_PIN_NUM_D04;
    panel_config.data_gpio_nums[13]=LCD_PIN_NUM_D05;
    panel_config.data_gpio_nums[14]=LCD_PIN_NUM_D06;
    panel_config.data_gpio_nums[15]=LCD_PIN_NUM_D07;
#endif
    memset(&panel_config.timings,0,sizeof(esp_lcd_rgb_timing_t));
    panel_config.timings.pclk_hz = LCD_PIXEL_CLOCK_HZ;
    panel_config.timings.h_res = LCD_HRES;
    panel_config.timings.v_res = LCD_VRES;
    panel_config.timings.hsync_back_porch = LCD_HSYNC_BACK_PORCH;
    panel_config.timings.hsync_front_porch = LCD_HSYNC_FRONT_PORCH;
    panel_config.timings.hsync_pulse_width = LCD_HSYNC_PULSE_WIDTH;
    panel_config.timings.vsync_back_porch = LCD_VSYNC_BACK_PORCH;
    panel_config.timings.vsync_front_porch = LCD_VSYNC_FRONT_PORCH;
    panel_config.timings.vsync_pulse_width = LCD_VSYNC_PULSE_WIDTH;
    panel_config.timings.flags.pclk_active_neg = true;
    panel_config.timings.flags.hsync_idle_low = false;
    panel_config.timings.flags.pclk_idle_high = LCD_CLK_IDLE_HIGH;
    panel_config.timings.flags.de_idle_high = LCD_DE_IDLE_HIGH;
    panel_config.timings.flags.vsync_idle_low = false;
    panel_config.flags.bb_invalidate_cache = true;
    panel_config.flags.disp_active_low = false;
    panel_config.flags.double_fb = false;
    panel_config.flags.no_fb = false;
    panel_config.flags.refresh_on_demand = false;
    panel_config.flags.fb_in_psram = true; // allocate frame buffer in PSRAM
    //panel_config.sram_trans_align = 4;
    //panel_config.psram_trans_align = 64;
    panel_config.num_fbs = 2;
    panel_config.bounce_buffer_size_px = LCD_HRES*(LCD_VRES/LCD_DIVISOR);
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));
    esp_lcd_rgb_panel_event_callbacks_t cbs;
    memset(&cbs,0,sizeof(cbs));
    cbs.on_color_trans_done = lcd_flush_complete;
    cbs.on_vsync = lcd_vsync;
    esp_lcd_rgb_panel_register_event_callbacks(panel_handle,&cbs,NULL);
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
#endif
#ifdef LCD_SPI_HOST
    spi_bus_config_t spi_cfg;
    memset(&spi_cfg,0,sizeof(spi_cfg));
    uint32_t spi_sz = (((LCD_WIDTH*(LCD_HEIGHT/LCD_DIVISOR)*LCD_BIT_DEPTH))+7)/8;
    if(spi_sz>32*1024) {
        spi_sz = 32*1024;
    }
    spi_cfg.max_transfer_sz = spi_sz;
    spi_cfg.mosi_io_num = LCD_PIN_NUM_MOSI;
    spi_cfg.sclk_io_num = LCD_PIN_NUM_CLK;
#ifdef LCD_PIN_NUM_MISO
    spi_cfg.miso_io_num = LCD_PIN_NUM_MISO;
#else    
    spi_cfg.miso_io_num = -1;
#endif
    spi_cfg.quadwp_io_num = -1;
    spi_cfg.quadhd_io_num = -1;
    ESP_ERROR_CHECK(spi_bus_initialize((spi_host_device_t)LCD_SPI_HOST,&spi_cfg,SPI_DMA_CH_AUTO));
    esp_lcd_panel_io_spi_config_t lcd_spi_cfg;
    memset(&lcd_spi_cfg,0,sizeof(lcd_spi_cfg));
    lcd_spi_cfg.cs_gpio_num = LCD_PIN_NUM_CS;
    lcd_spi_cfg.dc_gpio_num = LCD_PIN_NUM_DC;
    lcd_spi_cfg.lcd_cmd_bits = LCD_CMD_BITS;
    lcd_spi_cfg.lcd_param_bits = LCD_PARAM_BITS;        
#ifdef LCD_PIXEL_CLOCK_HZ
    lcd_spi_cfg.pclk_hz = LCD_PIXEL_CLOCK_HZ;
#else
    lcd_spi_cfg.pclk_hz = 20 * 1000 * 1000;
#endif
    lcd_spi_cfg.trans_queue_depth = 10;
    lcd_spi_cfg.on_color_trans_done = lcd_on_flush_complete;
    esp_lcd_panel_io_handle_t io_handle = NULL;
#ifdef LCD_SPI_MODE
    lcd_spi_cfg.spi_mode = LCD_SPI_MODE;
#else
    lcd_spi_cfg.spi_mode = 0;
#endif
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, &lcd_spi_cfg, &io_handle));
#endif
    esp_lcd_panel_dev_config_t panel_config;
    memset(&panel_config,0,sizeof(panel_config));
#ifdef LCD_PANEL_VENDOR_CONFIG
    LCD_PANEL_VENDOR_CONFIG;
#endif
#ifdef LCD_PIN_NUM_RST
    panel_config.reset_gpio_num = LCD_PIN_NUM_RST;
#else
    panel_config.reset_gpio_num = -1;
#endif
#if LCD_COLOR_SPACE == LCD_RGB
    panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
#elif LCD_COLOR_SPACE == LCD_BGR
    panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR;
#elif LCD_COLOR_SPACE == LCD_MONO
    // TODO: figure this out
    panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR;
#endif
#ifdef LCD_DATA_ENDIAN_LITTLE
    panel_config.data_endian = LCD_RGB_DATA_ENDIAN_LITTLE;
#else
    panel_config.data_endian = LCD_RGB_DATA_ENDIAN_BIG;
#endif
    panel_config.bits_per_pixel = LCD_BIT_DEPTH;
#ifdef LCD_PANEL_VENDOR_CONFIG
    panel_config.vendor_config = &vendor_config;
#else
    panel_config.vendor_config = NULL;
#endif
    ESP_ERROR_CHECK(LCD_PANEL(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    int gap_x = 0, gap_y = 0;
#ifdef LCD_GAP_X
    gap_x = LCD_GAP_X;
#endif
#ifdef LCD_GAP_Y
    gap_y = LCD_GAP_Y;
#endif
    if(gap_x!=0 || gap_y!=0) {
        ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle,gap_x,gap_y));
    }
#ifdef LCD_SWAP_XY
#if LCD_SWAP_XY
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle,true));
#endif
#endif
    bool mirror_x = false, mirror_y = false;
#ifdef LCD_MIRROR_X
#if LCD_MIRROR_X
    mirror_x = true;
#endif
#endif
#ifdef LCD_MIRROR_Y
#if LCD_MIRROR_Y
    mirror_y = true;
#endif
#endif
    if(mirror_x || mirror_y) {
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle,mirror_x,mirror_y));
    }
#ifdef LCD_INVERT_COLOR
#if LCD_INVERT_COLOR
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle,true));
#endif
#endif
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
#if LCD_PIN_NUM_BCKL >= 0
    gpio_set_level((gpio_num_t)LCD_PIN_NUM_BCKL, LCD_BCKL_ON_LEVEL);
#endif
    void *buf1 = NULL, *buf2=NULL;
    // it's recommended to allocate the draw buffer from internal memory, for better performance
    const size_t draw_buffer_sz = (((LCD_WIDTH*(LCD_HEIGHT/LCD_DIVISOR)*LCD_BIT_DEPTH))+7)/8;
    buf1 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    assert(buf1);
    buf2 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    assert(buf2);
    lcd.buffer_size(draw_buffer_sz);
    lcd.buffer1((uint8_t*)buf1);
    // 2nd buffer is for DMA performance, such that we can write one buffer while the other is transferring
    lcd.buffer2((uint8_t*)buf2);
    lcd.on_flush_callback(uix_flush,panel_handle);
}