# retro_clock

retro_clock is a retro inspired digital LCD clock that syncs to an NTP time server.

It exposes a configuration portal with which to set the network credentials, timezone and clock format.

![Clock screen](https://github.com/user-attachments/assets/50f5bc57-28e3-4b10-8506-20e38ac3a402 "Clock screen")

## Introduction

### Prerequisites

- You will need a Lilygo "TTGO" T-Display v1.1 or a compatible knock-off
- You will need VS Code with the PlatformIO extension installed
- You *might* need Python installed and added to your PATH for the pre-build step to work. (Though this isn't necessary unless you plan to modify the contents of the `www` folder)

While just for fun, this project hosts a wealth of tech behind a humble facade. It uses Truetype fonts, SVG, NTP, and a WiFi access point with a editable configuration portal all running under the auspices of the ESP-IDF.

For user interface and graphics it uses my http_uix (UIX) library, and the corresponding htcw_gfx (GFX) library which it builds on.

For sending pixels to the display it uses the ESP LCD Panel API.

The web portal content is delivered using the HTTPD facilities in the ESP-IDF and embedded into the firmware using my ClASP technology.

## Using This Mess

Using the clock is pretty easy. When you first turn it on you will presented with a configuration QR code that leads to a WiFi Access Point.

Once you use your phone to connect, the QR code will change, and you can use your phone once again, this time to navigate to the website given by the QR code.

From that website you will be able to configure your network credentials and clock. Once you submit the device will reset and the clock will appear and begin the sync process.

You can re-enter the portal at any time by hitting one of the two front buttons.

You can leave the portal at any time by resetting the device, although it will restart the portal if it doesn't have an existing configuration to work with.

## Coding This Mess

### The source tree

In `/` we have serveral files:

- `4MB.csv` contains partition tables for ESP32s with 4MB of flash.
- `8MB.csv` contains partition tables for ESP32s with 8MB of flash. (Not used yet)
- `16MB.csv` contains partition tables for ESP32s with 16MB of flash. (Not used yet)
- `article.md` is this file.
- `clasp_extra.py` is the script to invoke ClASP during the build in order to generate `include/httpd_content.h` from the contents of `/www`.
- `clasp.py`, `clasptree.py`, `clstat.py` and `visualfa.py` are all part of the ClASP suite used to generate C/++ content during the build process.
- `CMakeLists.txt` supports the ESP-IDF build process.
- `dark.css` is an alternative stylesheet for making a dark mode configuration portal page. (Not used)
- `LICENSE` contains the MIT License.
- `platformio.ini` contains the PlatformIO project configuration.
- `ports.ini` is an auxiliary PlatformIO configuration file used to set the serial ports for uploading and monitoring.
- `README.md` is the README for the Github repository.
- `sdkconfig.ttgo-t1` contains the configuration for the ESP-IDF on the TTGO

The `/data` folder is empty, and is simply present to allow Upload Filesystem Image in PIO to clear your configuration settings

The `/include` folder holds all of the headers we use for the various source components:

- `/include/assets` contains our TrueType fonts for our LCD segments, a TTF text font, and the connectivity WiFi SVG icon.
- `/include/captive_portal.h` is the interface to the captive portal component.
- `/include/config_input.h` is the interface to the component that enters the portal on input (for the TTGO, this is on button press).
- `/include/config.h` is the interface to the component which retreives and sets stored configuration values.
- `/include/dns_server.h` is the interface to the DNS server for the portal.
- `/include/httpd_content.h` is a ClASP generated file containing the embedded contents of the `/www` folder.
- `/include/httpd_epilogue.h` contains footer code appended to the end of each HTTP request.
- `/include/lcd_config.h` contains definitions for several display panels on various ESP32 devices. Currently only the TTGO_T1 section is used but more support may be added later.
- `/include/lcd.hpp` contains the interface for interacting with the display, including initialization.
- `/include/ntp.h` contains the interface for using the network time protocol.
- `/include/ui.hpp` contains the interface for the UI.
- `/include/wifi.h` contains the interface for using the WiFi radio.

The `/src` folder holds all of the translation unit implementation files for the firmware. The `CMakeLists.txt` file is for the ESP-IDF build process, and we'll ignore it.

- `/src/captive_portal.c` contains the implmentation for the configuration portal.
- `/src/config_input.c` contains the implementation for the input devices which lead to the configuration portal.
- `/src/config.c` contains the implementation for the configuration settings storage.
- `/src/dns_server.c` contains the DNS server implementation for the configuration portal.
- `/src/lcd.cpp` contains the implementation for the LCD display panel.
- `/src/main.cpp` contains the application entry point, basic initialization, and main loop(s).
- `/src/ntp.c` contains the implementation for the Network Time service
- `/src/wifi.c` contains the implementation for the WiFi radio interaction

The `/www` folder contains web content that is used by ClASP to generate `/include/httpd_content.h`

- `/www/default.css` contains the light-mode stylesheet used for the configuration portal.
- `/www/index.clasp` contains the dynamically rendered portal page. It is transformed into C/++ using [ClASP](https://github.com/codewitch-honey-crisis/clasp).

#### main.cpp

Let's start with `/src/main.cpp`:

First we include a bunch of files, many of which were mentioned above:

```cpp
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
```
Things get a little more interesting when we include the assets:

```cpp
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
```
Here you can define `SEG14` to use 14-segment LCD digits instead of the default 7-segment indicators.

Aside from that you'll notice `_IMPLEMENTATION` defines. What this is is an accounting for the fact that these files contain both the header and the implementation in a single file. It makes them easier to generate and to haul around between projects. However, to use them you must declare the associated define before including the file, in exactly one translation unit in your project. If you do not, it will not link. Aside from that, you use them like a normal header anywhere else.

There are several mentions of `const_buffer_stream` above. These wrap a static array in a stream. We don't use `<iostream>` because we don't rely on the STL. As such I have my own streams implementation as part of my embedded ecosystem. Here we're using it to wrap the font arrays so they can be consumed by GFX, and once again to alias the `telegram_render` font into `text_font` to make that easy to swap out.

The connectivity.hpp contains an SVG for a WiFi icon as a string.

Next up, some of our UI and graphics declarations and objects

```cpp
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
```

First we import `gfx` and `uix` namespaces since we use them frequently here.

Next we have some type aliases for the UI elements we'll be using. `surface_t` represents the screen's drawing surface, which is used to generate bitmaps to send the display. Every UIX control takes one as its first (or perhaps only) template argument. Specifically we have one for labels (used for text), one for icons (used for the wifi indicator), and one for the qr codes used for the configuration portal. Lastly, we have our `main_text_font` which is used for the main display elements, like the clock face, or the actual text during configuration. Initially it's set to very large (`LCD_HEIGHT`) and set to the `clock_font` but one or both of those will change later.

Next, some colors:
```cpp
constexpr static const auto back_color = color_t::white.blend(color_t::black, 0.5f);
constexpr static const auto ghost_color = ucolor_t::white.blend(ucolor_t::black, 0.42f);
constexpr static const auto text_color = ucolor_t::black;
```
Here we have the background color of the screen, the ghost color for the LCD segments, and the primary text color. You can see the first two were created by blending white with black. You'll note it's using the `color_t` and `ucolor_t` enumerations declared in `/include/ui.hpp`. They're two different types, because they're two different types of pixel format. The `back_color` is in the screen's native format (RGB565), while the other colors are in UIX format (RGBA8888).

Now we have some strings for the text that represents the ghosted segments:

```cpp

#ifndef SEG14
static const constexpr char* face_ghost_text = "88:88.";
static const constexpr char* face_ghost_text_mil = "88:88";
#else
static const constexpr char* face_ghost_text = "\x7E\x7E:\x7E\x7E.";
static const constexpr char* face_ghost_text_mil = "\x7E\x7E:\x7E\x7E";
#endif
```

They're different for the 14-seg LCD version.

Now here we hold our time information:

```cpp
static char time_buffer[7];
static int32_t time_offset = 0;
static time_t time_old = 0;
static time_t time_now = 0;
static bool time_military = false
```

The `time_buffer` field holds a string with the current time as text. The `time_offset` field holds the number of seconds offset from UTC. The `time_old` holds the previous time before the most recent update, while `time_now` hold the current time. These two allow for a differential test to see if the time has changed. `time_military` indicates whether or not the time is 24-hour (military time).

Next up is the part of our code that paints the WiFi icon. It paints it in the ghost color or the face color depending if the WiFi is active or not:

```cpp
bitmap<alpha_pixel<4>> wifi_icon;
void main_wifi_on_paint(surface_t& destination, const srect16& clip, void* state) {
    bool wifi_enabled = true;
    if(time_now>0) {
        wifi_enabled = ntp_syncing();
    }
    draw::icon(destination,spoint16::zero(),wifi_icon,wifi_enabled?text_color:ghost_color);
}
```

Here, `wifi_icon` is a `bitmap` with a 4-bit (16 values) alpha transparency map. This is used by the `draw::icon()` facility in GFX to render the icon at the indicated position (0,0) or the top left of the `main_wifi` control. Note that when `time_now` is 0 that means we're doing the initial WiFi negotation and connection process. 

The next bit of code handles updating the UI with the value of `time_now`:

```cpp
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
```
This routine takes the time and converts it to the appropriate string based on 24 hour/military time settings and adjusting for the `time_offset`. Note that when `time_now` is 0, again this is a special case where we're syncing. Above we don't apply the offset in that situation. One thing we do for 12 hour time is convert any leading zero to a `!` so that the segment doesn't show up, `!` being a special character for the font, that's a space exactly one digit in width. You may have noticed we set the `main_text` control using `time_buffer` rather than directly. UIX and GFX don't like to allocate memory unless you tell them to. When you give a label control a pointer to a string, it doesn't copy the memory. It points right at the memory you gave it - in this case `text_buffer`. That keeps things frugal.

Now we have a little helper function that creates a floating point rectangle out of an unsigned rectangle and an aspect ratio. This is so we can scale the wifi icon without distorting its aspect ratio:
```cpp
static rectf correct_aspect(const rect16& r, float aspect) {
    rect16 result = r;
    if (aspect>=1.f) {
        result.y2 /= aspect;
    } else {
        result.x2 *= aspect;
    }
    return (rectf)result;
}
```

While we could directly render SVG to the screen as needed, SVGs are not able to be rendered in different colors, because they themselves are color images. However, our WiFi icon is strictly black and white. We can leverage that by creating an *alpha transparency map* - a bitmap where each pixel indicates how opaque the draw should be at that position. We can then use that map to draw in any color we give it. The next routine handles creating one of these alpha transparency maps, `wifi_icon` from the SVG. To render that SVG we created a `bitmap`, bound a vector `canvas` to it, and then rendered the SVG contents using that.

```cpp
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
```

Now a segue into a bit of NTP handling. The following is a callback method that reports the new time whenever it gets updated via NTP. Here we just report it, update our time values, and invalidate the `main_wifi` control to force it to repaint.

```cpp
void ntp_on_sync(time_t val, void* state) {
    puts("Time synced");
    time_old = time_now;
    time_now = val;
    main_wifi.invalidate();
}
```

Next up we have some state to hold our WiFi info, including the SSID and password of the access point, and state where we determine whether or not it's connected. Again we keep the old value so we can do a differenetial comparison to be notified when the state changes:
```cpp
static char wifi_ssid[65];
static char wifi_pass[129];
static bool wifi_connected_old = false;
static bool wifi_connected = false;
```

Now we're on to the real meat - the clock application logic itself. It's not actually that complicated, but there's a lot to cover here all at once so I'll break it down into parts. It should be noted that in addition to the clock application mode, there is also the configuration portal mode, so we have different routines each one, in case you're wondering why this isn't the entry point:

```cpp
static void clock_app(void) {
    time_offset = 0;
    char buf[65];
    if(config_get_value("tzoffset",0,buf,sizeof(buf)-1)) {
        sscanf(buf,"%ld",&time_offset);
    }
    time_military = config_get_value("military",0,nullptr,0);
```

The first thing we do is get some configuration values. If `tzoffset` is preset (meaning */spiffs/tzoffset.txt* exists) it will parse that into the `time_offset` value. If `military` exists, it sets the `time_military` flag. We could have called it 24-hour format but there would have been cases where I would have had to prefix "24" with something to make it a legal identifier. I chose not to muddy the water with that, so I chose to use "military" nomenclature instead.

I've made the interface scalable, such that it will size itself to the display's resolution. To that end, we must compute the appropriate size for the clock face font. We do that next by initializing `main_text_font` and then steadily decreasing the size of it until it fits within 80% of the display's width given the "88:88." text (or the appropriate variant) we use for the ghosted segments.
```cpp
main_text_font.initialize();
// find the appropriate size for the font
text_info face_ti(time_military?face_ghost_text_mil:face_ghost_text, main_text_font);
size16 face_area;
main_text_font.measure((uint16_t)-1, face_ti, &face_area);
while (face_area.width == 0 || face_area.width > (LCD_WIDTH * .8)) {
    main_text_font.size(main_text_font.line_height() - 1, font_size_units::px);
    main_text_font.measure((uint16_t)-1, face_ti, &face_area);
}
```

Next we call the wifi icon rasterization routine covered prior:

```cpp
// rasterize the wifi icon
create_wifi_icon(LCD_WIDTH*.12);
```

Now it's time to lay out the controls and configure them. In turn we set the bounding box and "properties" of the `main_ghost` (the background segments) label, the `main_text` label (here used for the face segments that are "on"), and the `main_wifi` painter box. We register each of the controls with the screen after setting them up. Finally, we set the active screen for the `lcd` object to the `main_screen`:

```cpp
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
```

The next bit is some state we use in our application's primary loop:

```cpp
TickType_t ticks_flash = xTaskGetTickCount();
TickType_t ticks_wdt = xTaskGetTickCount();
TickType_t ticks_time_retry = 0;
TickType_t ticks_time = xTaskGetTickCount();
```
These are timestamps we use for when the face needs to flash during initial sync, for resetting the ESP-IDF watchdog timer to prevent a reboot, for retrying the NTP sync if the initial sync fails, and for counting off each second so we limit the amount we actually update the time, respectively.

Next is our main application loop for the clock, where first we keep the WiFi connection state and NTP syncing up to date:

```cpp
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
```

Now we pop off various "events" when certain periods elapse:

```cpp
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
```
The first one feeds the watchdog timer by calling `vTaskDelay(5)` every 200ms or so to prevent the watchdog from causing a reboot.
Next we handle the case where we're still syncing (`time_now` is zero) wherein every 500ms we set the face text to visible or not, causing a flashing effect, after which we retry the NTP sync every 30 seconds.

The next one handles post sync operation (`time_now` is non-zero) where we first update the time every second, and then we just ensure `main_text` is visible in case it was off because we were flashing it before.

For the final part of the application loop we just poll the `config_input` subsystem to enter the config portal, and keep the `lcd` object up to date.
```cpp
config_input_update();
lcd.update();
```
That's the entire clock operation. Now we get to move on to some portal magic.

```cpp
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
```
This handles the screen transition from "Connect" to "Configure" after the user connects to the exposed WiFi Access Point.

Now let's move on to the main portal code. The following starts off by clearing the "configure" configuration value if it is set. When that value is present it signals to the device that the next boot will be into the configuration portal. Since we're here now the first thing we do is delete it. After that we initialize the portal and get the credentials of the access point:

```cpp
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
```
After that, similar to how we did in the clock app, we take a font and reduce the size until the width fits within our parameters:

```cpp
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
```
Next we initialize the text that goes to the left of the QR code:

```cpp
main_screen.background_color(color_t::light_gray);
main_text.bounds(srect16(spoint16::zero(),ssize16(LCD_WIDTH/2,main_text_font.line_height())).center_vertical(main_screen.bounds()));
main_text.padding({10,0});
main_text.font(main_text_font);
main_text.color(ucolor_t::black);
main_text.text("Connect:");
main_text.text_justify(uix_justify::center_right);
main_screen.register_control(main_text);
```
Then we initialize the QR code with the access point website's URL:

```cpp
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
```

Finally, we set the screen and update the display.
```cpp
lcd.active_screen(main_screen);
lcd.update();
```

