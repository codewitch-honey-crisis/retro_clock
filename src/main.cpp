#include "esp_check.h"
#include "esp_idf_version.h"
#include "esp_spiffs.h"
#include "esp_vfs_fat.h"
#include "esp_random.h"
#include <sys/time.h>   
#include <time.h>
#include <string.h>
#include <stdio.h>
#include "config.h"
#include "config_input.h"
#include "captive_portal.h"
#include "ui.hpp"
#include "lcd.hpp"
#include "ntp.h"
#include "wifi.h"
// from https://www.keshikan.net/fonts-e.html
#ifdef SEG14
#define DSEG14CLASSIC_REGULAR_IMPLEMENTATION
#include "assets/DSEG14Classic_Regular.hpp"
static gfx::const_buffer_stream clock_font(DSEG14Classic_Regular, sizeof(DSEG14Classic_Regular));
#else
#define DSEG7CLASSIC_REGULAR_IMPLEMENTATION
#include "assets/DSEG7Classic_Regular.hpp"
static gfx::const_buffer_stream clock_font(DSEG7Classic_Regular, sizeof(DSEG7Classic_Regular));
#endif
// from https://fontsquirrel.com
#define TELEGRAMA_RENDER_IMPLEMENTATION
#include "assets/telegrama_render.hpp"
gfx::const_buffer_stream& text_font = telegrama_render;
#define CONNECTIVITY_IMPLEMENTATION
#include "assets/connectivity.hpp"

using namespace gfx;
using namespace uix;

using label_t = label<surface_t>;
using icon_t = painter<surface_t>;
using qr_t = qrcode<surface_t>;

screen_t main_screen;
label_t main_text;
label_t main_ghost;
icon_t main_wifi;
qr_t main_qr;

tt_font main_text_font(clock_font, LCD_HEIGHT, font_size_units::px);

constexpr static const auto back_color = color_t::white.blend(color_t::black, 0.5f);
constexpr static const auto ghost_color = ucolor_t::white.blend(ucolor_t::black, 0.42f);
constexpr static const auto text_color = ucolor_t::black;

#ifndef SEG14
static const constexpr char* face_ghost_text = "88:88.";
static const constexpr char* face_ghost_text_mil = "88:88";
#else
static const constexpr char* face_ghost_text = "\x7E\x7E:\x7E\x7E.";
static const constexpr char* face_ghost_text_mil = "\x7E\x7E:\x7E\x7E";
#endif

static char time_buffer[7];
static int32_t time_offset = 0;
static time_t time_old = 0;
static time_t time_now = 0;
static bool time_military = false;

bitmap<alpha_pixel<4>> wifi_icon;
void main_wifi_on_paint(surface_t& destination, const srect16& clip, void* state) {
    bool wifi_enabled = true;
    if(time_now>0) {
        wifi_enabled = ntp_syncing();
    }
    draw::icon(destination,spoint16::zero(),wifi_icon,wifi_enabled?text_color:ghost_color);
}

void time_update() {
    time_t time_offs = time_now==0?0:(time_t)(time_now + time_offset);
    tm tim = *localtime(&time_offs);
    bool dot = 0 == (time_now & 1);
    if (dot) {
        if (!time_military) {
            if (tim.tm_hour >= 12) {
                strftime(time_buffer, sizeof(time_buffer), "%I:%M.", &tim);
            } else {
                strftime(time_buffer, sizeof(time_buffer), "%I:%M", &tim);
            }
            if (*time_buffer == '0') {
                *time_buffer = '!';
            }
        } else {
            strftime(time_buffer, sizeof(time_buffer), "%H:%M", &tim);
        }
    } else {
        if (!time_military) {
            if (tim.tm_hour >= 12) {
                strftime(time_buffer, sizeof(time_buffer), "%I %M.", &tim);
            } else {
                strftime(time_buffer, sizeof(time_buffer), "%I %M", &tim);
            }
            if (*time_buffer == '0') {
                *time_buffer = '!';
            }
        } else {
            strftime(time_buffer, sizeof(time_buffer), "%H %M", &tim);
        }
    }
    main_text.text(time_buffer);
}

static rectf correct_aspect(const rect16& r, float aspect) {
    rect16 result = r;
    if (aspect>=1.f) {
        result.y2 /= aspect;
    } else {
        result.x2 *= aspect;
    }
    return (rectf)result;
}
static bool create_wifi_icon(uint16_t icon_size) {
    // create a new bitmap in 4-bit grayscale
    auto bmp = create_bitmap<gsc_pixel<4>>({icon_size,icon_size});
    if(bmp.begin()==nullptr) {
        // out of memory
        return false;
    }
    // fill with white
    bmp.fill(bmp.bounds(),gsc_pixel<4>(15));
    // assign the bitmap array entry to the bitmap's buffer
    canvas cvs(bmp.dimensions());
    if(gfx_result::success!=cvs.initialize()) {
        // out of memory
        free(bmp.begin());
        return false;
    }
    // link the canvas and the bitmap so the canvas can draw on it
    if(gfx_result::success!=draw::canvas(bmp,cvs)) {
        // can't imagine why this would fail
        free(bmp.begin());
        return false;
    }
    sizef cps = connectivity_wifi_dimensions;
    // scale it
    gfx::rectf corrected = correct_aspect(bmp.bounds(), cps.aspect_ratio());
    // center it
    corrected.center_inplace((gfx::rectf)bmp.bounds());
    // create a transformation matrix using it to fit to the bounding box
    matrix fit = matrix::create_fit_to(cps,corrected);
    connectivity_wifi.seek(0); // make sure we're at the beginning
    if(gfx_result::success!=cvs.render_svg(connectivity_wifi,fit)) {
        puts("Error rasterizing SVG");
        goto error;
    }
    cvs.deinitialize();
    // invert, because it's black on white, but we need an alpha transparency map
    for(int y = 0;y<bmp.dimensions().height;++y) {
        for(int x = 0;x<bmp.dimensions().width;++x) {
            point16 pt(x,y);
            decltype(bmp)::pixel_type px;
            bmp.point(pt,&px);
            // get the channel color value and invert it by subtracting from its maximum allowable value
            px.channel<0>(decltype(px)::channel_by_index<0>::max-px.channel<0>());
            bmp.point(pt,px);
        }    
    }
    // wrap the memory we just converted with an alpha transparency map.
    wifi_icon = bitmap<decltype(wifi_icon)::pixel_type>(bmp.dimensions(),bmp.begin());
    return true;
error:
    if(bmp.begin()!=nullptr) {
        free(bmp.begin());
    }
    puts("Error creating WiFi icon");
    return false;
}
void ntp_on_sync(time_t val, void* state) {
    puts("Time synced");
    time_old = time_now;
    time_now = val;
    main_wifi.invalidate();
}
static char wifi_ssid[65];
static char wifi_pass[129];
static bool wifi_connected_old = false;
static bool wifi_connected = false;
static void clock_app(void) {
    time_offset = 0;
    char buf[65];
    if(config_get_value("tzoffset",0,buf,sizeof(buf)-1)) {
        sscanf(buf,"%ld",&time_offset);
    }
    time_military = config_get_value("military",0,nullptr,0);
    main_text_font.initialize();
    // find the appropriate size for the font
    text_info face_ti(time_military?face_ghost_text_mil:face_ghost_text, main_text_font);
    size16 face_area;
    main_text_font.measure((uint16_t)-1, face_ti, &face_area);
    while (face_area.width == 0 || face_area.width > (LCD_WIDTH * .8)) {
        main_text_font.size(main_text_font.line_height() - 1, font_size_units::px);
        main_text_font.measure((uint16_t)-1, face_ti, &face_area);
    }
    // rasterize the wifi icon
    create_wifi_icon(LCD_WIDTH*.12);

    main_screen.background_color(back_color);
    main_ghost.bounds(srect16(spoint16::zero(), (ssize16)face_area).center(main_screen.bounds()));
    main_ghost.padding({0, 0});
    main_ghost.text_justify(uix_justify::top_left);
    main_ghost.color(ghost_color);
    main_ghost.text(face_ti.text,face_ti.text_byte_count);
    main_ghost.font(main_text_font);
    main_screen.register_control(main_ghost);

    main_text.bounds(srect16(spoint16::zero(), (ssize16)face_area).center(main_screen.bounds()));
    main_text.padding({0, 0});
    main_text.text_justify(uix_justify::top_left);
    main_text.color(text_color);
    main_text.font(main_text_font);
    main_screen.register_control(main_text);
    // set the initial time value
    time_update();

    main_wifi.bounds(srect16(spoint16::zero(),(ssize16)wifi_icon.dimensions()).offset(LCD_WIDTH-wifi_icon.dimensions().width,0));
    main_wifi.on_paint_callback(main_wifi_on_paint);
    main_screen.register_control(main_wifi);

    lcd.active_screen(main_screen);

    TickType_t ticks_flash = xTaskGetTickCount();
    TickType_t ticks_wdt = xTaskGetTickCount();
    TickType_t ticks_time_retry = 0;
    TickType_t ticks_time = xTaskGetTickCount();
    while (1) {
        switch (wifi_status()) {
            case WIFI_DISCONNECTED:
                fputs("Connecting to ",stdout);
                puts(wifi_ssid);
                wifi_init(wifi_ssid, wifi_pass);
                main_wifi.invalidate();
                break;
            case WIFI_CONNECTED:
                wifi_connected = true;
                if (!wifi_connected_old) {
                    main_wifi.invalidate();
                    ntp_on_sync_callback(ntp_on_sync, nullptr);
                    ntp_init();
                    ticks_time_retry = xTaskGetTickCount();
                }
                break;
            case WIFI_CONNECT_FAILED:
                wifi_restart();
                break;
            default:
                break;
        }
        wifi_connected_old = wifi_connected;

        if (xTaskGetTickCount() > (ticks_wdt + pdMS_TO_TICKS(200))) {
            ticks_wdt = xTaskGetTickCount();
            vTaskDelay(5);
        }
        if (time_now == 0) {
            if (xTaskGetTickCount() > (ticks_flash + pdMS_TO_TICKS(500))) {
                ticks_flash = xTaskGetTickCount();
                main_text.visible(!main_text.visible());
                time_update();
            }
            if(xTaskGetTickCount() > (ticks_time_retry+pdMS_TO_TICKS(30 * 1000))) {
                ticks_time_retry = xTaskGetTickCount();
                puts("Retry NTP sync");
                ntp_sync();
            }
        } else {
            if (xTaskGetTickCount() > (ticks_time + pdMS_TO_TICKS(1000))) {
                time_old = time_now;
                ticks_time = xTaskGetTickCount();
                timeval tv;
                gettimeofday(&tv,NULL);
                time_now = (time_t)tv.tv_sec;
                time_update();
            }
            main_text.visible(true);
        }
        config_input_update();
        lcd.update();
    }
}
static char portal_address[257];
static void portal_on_connect(void* state) {
    if(!captive_portal_get_address(portal_address,sizeof(portal_address)-1)) {
        puts("Failed to get portal address");
        return;
    }
    main_text.text("Configure:");
    main_qr.color(ucolor_t::dark_blue);
    main_qr.text(portal_address);
    lcd.update();
}
char qr_text[513];
static void portal_app(void) {
    config_clear_values("configure");
    captive_portal_on_sta_connect(portal_on_connect,nullptr);
    if(!captive_portal_init()) {
        puts("Failed to initialize captive portal");
        return;
    }
    if(!captive_portal_get_credentials(wifi_ssid,sizeof(wifi_ssid)-1,wifi_pass,sizeof(wifi_pass)-1)) {
        puts("Failed to get captive portal credentials");
        return;
    }
    main_text_font   = tt_font(text_font,LCD_HEIGHT,font_size_units::px);
    main_text_font.initialize();
    // find the appropriate size for the font
    text_info face_ti("Configure:", main_text_font);
    size16 face_area;
    main_text_font.measure((uint16_t)-1, face_ti, &face_area);
    while (face_area.width == 0 || face_area.width > (LCD_WIDTH * .4)) {
        main_text_font.size(main_text_font.line_height() - 1, font_size_units::px);
        main_text_font.measure((uint16_t)-1, face_ti, &face_area);
    }
    main_screen.background_color(color_t::light_gray);
    main_text.bounds(srect16(spoint16::zero(),ssize16(LCD_WIDTH/2,main_text_font.line_height())).center_vertical(main_screen.bounds()));
    main_text.padding({10,0});
    main_text.font(main_text_font);
    main_text.color(ucolor_t::black);
    main_text.text("Connect:");
    main_text.text_justify(uix_justify::center_right);
    main_screen.register_control(main_text);
    main_qr.bounds(srect16(spoint16(LCD_WIDTH/2,0),ssize16(LCD_WIDTH/2,LCD_HEIGHT)));
    qr_text[0]='\0';
    if(!captive_portal_get_ap_address(qr_text,sizeof(qr_text)-1)) {
        puts("Failed to get captive portal address");
        captive_portal_end();
        return;
    }
    main_qr.text(qr_text);
    main_qr.color(ucolor_t::black);
    main_screen.register_control(main_qr);
    lcd.active_screen(main_screen);
    lcd.update();
}
// generate a friendly AP password/identifier for this device
static void gen_device_id() {
    char id[16];
    static const char* vowels = "aeiou";
    char tmp[2] = {0};
    size_t len = esp_random()%4+7;
    int salt = esp_random()%98+1;
    bool vowel = esp_random()&1?true:false;
    id[0]='\0';
    for(int i = 0;i<len;++i) {
        if(vowel) {
            int idx = esp_random()%5;
            tmp[0]=vowels[idx];
        } else {
            char ch = (esp_random()%26)+'a';
            while(strchr(vowels,ch)) {
                ch = (esp_random()%26)+'a';
            }
            tmp[0]=ch;
        }
        strcat(id,tmp);
        if(!vowel) {
            vowel=true;
        } else {
            vowel = esp_random()%10>6?true:false;
        }
    }
    itoa(salt,id+strlen(id),10);
    config_clear_values("deviceid");
    config_add_value("deviceid",id);
}

static void spiffs_init(void) {
    esp_vfs_spiffs_conf_t conf;
    memset(&conf, 0, sizeof(conf));
    conf.base_path = "/spiffs";
    conf.partition_label = NULL;
    conf.max_files = 5;
    conf.format_if_mount_failed = true;
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
}

extern "C" void app_main(void) {
    lcd_init();
    spiffs_init();
    config_input_init();
    if(!config_get_value("deviceid",0,NULL,0)) {
        gen_device_id();
    }
    bool cfg = config_get_value("configure",0,NULL,0);
    main_screen.dimensions({LCD_WIDTH, LCD_HEIGHT});
    if(cfg) {
        portal_app();
        return;
    }
    wifi_ssid[0] = 0;
    wifi_pass[0] = 0;
    puts("Looking for wifi.txt creds on internal flash");
    bool has_creds = false;
    has_creds = wifi_load("/spiffs/wifi.txt", wifi_ssid, wifi_pass);
    if (has_creds) {
        clock_app();
    } else {
        portal_app();
    }
}
