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
static bool time_military = false;
```

The `time_buffer` field holds a string with the current time as text. The `time_offset` field holds the number of seconds offset from UTC. `time_old` holds the previous time before the most recent update, while `time_now` hold the current time. These two allow for a differential test to see if the time has changed. `time_military` indicates whether or not the time is 24-hour (military time).

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

While we could directly render SVG to the screen as needed, SVGs are not able to be rendered in different colors, because they themselves are color images. However, our WiFi icon is strictly black and white. We can leverage that by creating an *alpha transparency map* - a bitmap where each pixel indicates how opaque the draw should be at that position. We can then use that map to draw in any color we give it. The next routine handles creating one of these alpha transparency maps, `wifi_icon` from the SVG. To render that SVG we created a `bitmap`, bound a vector `canvas` to it, and then rendered the SVG contents using that:

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

Now a segue into a bit of NTP handling. The following is a callback method that reports the new time whenever it gets updated via NTP. Here we just report it, update our time values, and invalidate the `main_wifi` control to force it to repaint:

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

Next we call the WiFi icon rasterization routine covered prior:

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

For the final part of the application loop we just poll the `config_input` subsystem to enter the config portal, and keep the `lcd` object up to date:
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

Finally, we set the screen and update the display:
```cpp
lcd.active_screen(main_screen);
lcd.update();
```

That's it for the portal, now we're just dealing with initialization and entry point mechanics.

The password for the access point is not really for security. The only thing it prevents is someone hijacking your configuration portal from say, next door. It's not meant to be strong, or provide anything more than a cursory check against random people fiddling with your portal. What I've done is decided that this thing should be unique, but short-ish and friendly to remember.

To that end, we use the ESP32s entropy source RNG and create a string with vowels and consonants and a number on the end, like "foozbart12". It then stores this as a deviceid configuration value.
Presently we only use it as an AP password, but it could be used elsewhere because as I said, it's not really for security:

```cpp
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
```

We have to initialize SPIFFS before we can do much:

```cpp
static void spiffs_init(void) {
    esp_vfs_spiffs_conf_t conf;
    memset(&conf, 0, sizeof(conf));
    conf.base_path = "/spiffs";
    conf.partition_label = NULL;
    conf.max_files = 5;
    conf.format_if_mount_failed = true;
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
}
```

In the entry point we do core initialization, generate the deviceid, and then determine if we need to enter the portal or the clock app:

```cpp
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
```

That's it for main.cpp. Let's look at the `src/captive_portal.c` next:

#### captive_portal.c

Calling it a "captive portal" is somewhat wishful thinking. Without HTTPS you aren't getting off the ground in that regard with most phones, which default to it. However, maintaining a certificate for this hardware is a whole ball of wax that I don't want to take on. Furthermore captive portals are only kind of supported by phones, and it's dodgy. This codebase flew too close to the sun, and is named accordingly. I have not renamed it for historic reasons (I use it in other projects), and plus I intend to revisit this and potentially shore up the shortcomings that prevent it from being a full captive portal in the future. First some includes and defines (boilerplate):

```c
#ifdef ESP_PLATFORM
#include "captive_portal.h"

#include <memory.h>
#include <string.h>
#include <sys/param.h>

#include "common.h"
#include "dns_server.h"
#include "config.h" 
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "lwip/inet.h"
#include "nvs_flash.h"
#include "esp_random.h"

#define STRINGIFY(s) STRINGIFY1(s)
#define STRINGIFY1(s) #s
#ifndef CAPTIVE_PORTAL_SSID
#define CAPTIVE_PORTAL_SSID ClockPortal
#endif
#define CAPTIVE_PORTAL_SSID1 STRINGIFY(CAPTIVE_PORTAL_SSID)
#define MAX_STA_CONN 5
static const char* TAG = "portal";
```
The first `#ifdef` is because it's conditionally only compiled for the ESP32 devices because it only works on them, and as I said I have used it in other projects as well. After that we have a bunch of `#include`s and some funky macros so that we can set CAPTIVE_PORTAL_SSID during the build if we want it to be something other than "ClockPortal". Finally we have the `TAG` which is used for logging.

Next we have a structure that holds information we use in our HTTP response handlers:

```c
typedef struct {
    httpd_handle_t hd;
    int fd;
    char path_and_query[513];
} httpd_async_resp_arg_t;
```
The first two fields are used by our socket writing code to send data over the connected TCP socket. The `path_and_query` field is used by our code inside the request handler(s) as necessary.

These two prototypes are used by our response handlers generated in `/include/httpd_content.h` to write data out to a socket. Here `arg` would end up being an instance of the structure just above:
```c
static void httpd_send_block(const char *data, size_t len, void *arg);
static void httpd_send_expr(const char* data, void *arg);
```
The next two prototypes are used inside our SSR page `/www/index.clasp` for the portal:
```c
static const char* httpd_crack_query(const char* url_part, char* name,
                                     char* value);
static char* httpd_url_decode(char* dst, size_t dstlen, const char* src);
```
Now we include the ClASP generated content, including its implementation code into this file:
```c
#define HTTPD_CONTENT_IMPLEMENTATION
#include "httpd_content.h"
```
The code therein is generated from the contents of `/www` as part of the build process.

Next up, we need a bunch of state we use to track the WiFi information:

```c
static bool wifi_intialized = false;
static httpd_handle_t httpd_handle = NULL;
static dns_server_handle_t dns_handle = NULL;
static esp_netif_t* wifi_ap = NULL;
static char* captive_portal_uri = NULL;
static char wifi_ssid[129];
static char wifi_pass[16];
static wifi_ap_record_t wifi_ap_info[256];
static size_t wifi_ap_info_size = 0;
```

We can't always know that there won't be another "ClockPortal" AP already in range of your phone, so instead we scan the APs on startup and if there already is one, we name ours ClockPortal2, or ClockPortal3 and so on as necessary. The following routine facilitates that:

```c
static void scan_for_wifi_ssid(void)
{
    strcpy(wifi_ssid,CAPTIVE_PORTAL_SSID1);
    char target[129];
    strcpy(target,CAPTIVE_PORTAL_SSID1);
    bool wifi_inited = false;


    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    if(sta_netif==NULL) {
        goto error;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if(ESP_OK!=esp_wifi_init(&cfg)) {
        return;
    }
    uint16_t ap_max = 256;
    uint16_t ap_count = 0;
    memset(wifi_ap_info, 0, sizeof(wifi_ap_info));

    if(ESP_OK!=esp_wifi_set_mode(WIFI_MODE_STA)) {
        goto error;
    }
    if(ESP_OK!=esp_wifi_start()) {
        goto error;
    }
    wifi_inited = true;

    if(ESP_OK!=esp_wifi_scan_start(NULL, true)) {
        ESP_LOGE(TAG,"Error initiating scan");
        goto error;
    }

    if(ESP_OK!=esp_wifi_scan_get_ap_num(&ap_count)) {
        ESP_LOGE(TAG,"Error retreiving ap count");
        goto error;
    }
    if(ESP_OK!=esp_wifi_scan_get_ap_records(&ap_max, wifi_ap_info)) {
        ESP_LOGE(TAG,"Error scanning records");
        esp_wifi_clear_ap_list();
        goto error;
    }
    wifi_ap_info_size = ap_max;
    int result = 1;
    bool done = false;
    while(!done) {
            done = true;
            for(int i = 0;i<ap_max;++i) {
            if(0==strcmp((char*)wifi_ap_info[i].ssid,target)) {
                strcpy(target,CAPTIVE_PORTAL_SSID1);
                itoa(++result,target+strlen(target),10);
                done = false;
            }
        }
    }
    esp_wifi_scan_stop();
    esp_wifi_disconnect();
    
    ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u, result = %s", ap_count, ap_max,target);
    
    strcpy(wifi_ssid,target);
error:
    if(wifi_inited) {
        esp_wifi_stop();
        esp_wifi_deinit();
        
    }
}
```
It also stashes those APs for later so we can populate the website portal with them, although it's not presently done in this code. To get those APs you'd use the following two functions inside a .clasp page:

```c
size_t captive_portal_get_ap_list_size() {
    return wifi_ap_info_size;
}
const char* captive_portal_get_ap_list_ssid(size_t index) {
    if(index >= wifi_ap_info_size) {
        return NULL;
    }
    return (const char*)wifi_ap_info[index].ssid;
}
```
The following code gives us the opportunity to log WiFi AP connection and disconnection:
```c
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
}
```
This code tears down the WiFi. Note that we use different WiFi handling in this code than in wifi.h because here we're exposing an AP rather than connecting to one:

```c
static void wifi_deinit_softap(void) {
    if(!wifi_intialized) {
        return;
    }
    esp_wifi_stop();
    esp_event_handler_unregister(WIFI_EVENT,ESP_EVENT_ANY_ID,&wifi_event_handler);
    esp_wifi_deinit();
    wifi_intialized = false;
}
```
The initialization of the WiFi AP is a bit of a slog. Here it is:
```c
static bool wifi_init_softap(void) {
    if(wifi_intialized) {
        return true;
    }
    config_get_value("deviceid",0,wifi_pass,sizeof(wifi_pass)-1);
    bool wifi_init = false;
    bool event_handler_init = false;
    bool wifi_started = false;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (ESP_OK != esp_wifi_init(&cfg)) {
        return false;
    }
    wifi_init = true;
    if (ESP_OK != esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL)) {
        goto error;
    }
    event_handler_init=true;
    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = strlen(wifi_ssid),
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK},
    };
    strcpy((char*)wifi_config.ap.ssid,wifi_ssid);
    strcpy((char*)wifi_config.ap.password,wifi_pass);
    // if (strlen(ESP_WIFI_PASS) == 0) {
    //     wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    // }

    if(ESP_OK!=esp_wifi_set_mode(WIFI_MODE_AP)) {
        goto error;
    }
    if(ESP_OK!=esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config)) {
        goto error;
    }
    if(ESP_OK!=esp_wifi_start()) {
        goto error;
    }
    wifi_started = true;
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);

    char ip_addr[16];
    inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);
    ESP_LOGI(TAG, "Set up softAP with IP: %s", ip_addr);

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:'%s' password:'%s'",
             wifi_ssid, wifi_pass);
    wifi_intialized = true;
    return true;
error:
    if(wifi_started) {
        esp_wifi_stop();
    }
    if(event_handler_init) {
        esp_event_handler_unregister(WIFI_EVENT,ESP_EVENT_ANY_ID,&wifi_event_handler);
    }
    if(wifi_init) {
        esp_wifi_deinit();
    }
    return false;
}
```
We also run our own DHCP service off of our AP:

```c
static bool dhcp_set_captiveportal_url(void) {
    if(captive_portal_uri!=NULL) {
        free(captive_portal_uri);
    }
    // get the IP of the access point to redirect to
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);

    char ip_addr[16];
    inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);
    ESP_LOGI(TAG, "Set up softAP with IP: %s", ip_addr);

    // turn the IP into a URI
    captive_portal_uri = (char*)malloc(32 * sizeof(char));
    if (captive_portal_uri == NULL) {
        return false;
    }
    strcpy(captive_portal_uri, "http://");
    strcat(captive_portal_uri, ip_addr);

    // get a handle to configure DHCP with
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    // set the DHCP option 114
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(netif));
    if (ESP_OK != esp_netif_dhcps_option(netif, ESP_NETIF_OP_SET, ESP_NETIF_CAPTIVEPORTAL_URI, captive_portal_uri, strlen(captive_portal_uri))) {
        free(captive_portal_uri);
        return false;
    }
    if (ESP_OK != esp_netif_dhcps_start(netif)) {
        free(captive_portal_uri);
        return false;
    }
    return true;
}
```
The following function is used by the HTTP request handler to process URL query parameters and update configuration values, accordingly, and then restarting after they are committed:

```c
// parse the query parameters and apply them to the settings.
static void parse_url_and_apply(const char* url) {
    const char* query = strchr(url, '?');
    char ssid[64];
    ssid[0]='\0';
    char pass[64];
    pass[0]='\0';
    char tz[64];
    char military[64];
    char name[64];
    char value[64];
    bool restart = false;
    bool set_creds = false;
    bool set_tz = false;
    bool set_military = false;
    bool configured = false;
    if (query != NULL) {
        while (1) {
            query = httpd_crack_query(query, name, value);
            if (!query) {
                break;
            }
            if(0==strcmp("ssid",name)) {
                set_creds=true;
                httpd_url_decode(ssid,sizeof(ssid),value);
            }
            if(0==strcmp("pass",name)) {
                set_creds = true;
                httpd_url_decode(pass,sizeof(pass),value);
            }
            if(0==strcmp("tzoffset",name)) {
                set_tz=true;
                strncpy(tz,value,63);
            }
            if(0==strcmp("military",name)) {
                set_military=true;
                strncpy(military,value,63);
            }
            if(0==strcmp("configure",name)) {
                configured=true;
            }
        }
    }
    
    if(set_creds) {
        config_clear_values("wifi");
        config_add_value("wifi",ssid);
        config_add_value("wifi",pass);
        restart = true;
    }
    if(set_tz) {
        float ofs = 0;
        sscanf(tz,"%f",&ofs);
        char buf[32];
        snprintf(buf,31,"%ld",(long)ofs);
        config_clear_values("tzoffset");
        config_add_value("tzoffset",buf);
        restart=true;
    }
    bool old_military = config_get_value("military",0,NULL,0);
    if(set_military) {
        config_clear_values("military");
        config_add_value("military","1");
        if(!restart) restart=set_military!=old_military;
    } else {
        config_clear_values("military");
        if(!restart) restart=set_military!=old_military;
    }
    if(configured) {
        if(restart) {
            puts("Configuration saved. Restarting");
            esp_restart();
        }
    }
}
```
Admittedly this code can probably be simplified, but doing so requires some time for testing and such, which I'll get to.

On an error we just send our portal page again. It may be better to send an actual redirect, I am not sure:

```c
static esp_err_t httpd_err_handler(httpd_req_t *req, httpd_err_code_t error) {
    ESP_LOGI(TAG,"Sending captive portal redirect");
    httpd_async_resp_arg_t* resp_arg = (httpd_async_resp_arg_t*)malloc(sizeof(httpd_async_resp_arg_t));
    if (resp_arg == NULL) {
        ESP_LOGE(TAG,"No memory for HTTP request!");
        return ESP_ERR_NO_MEM;
    }
    resp_arg->fd = httpd_req_to_sockfd(req);
    if (resp_arg->fd == -1) {
        free(resp_arg);
        return ESP_FAIL;
    }
    resp_arg->hd = httpd_handle;
    ESP_LOGE(TAG,"Queueing HTTP response");
    return httpd_queue_work(httpd_handle,httpd_content_index_clasp, resp_arg);
}
```
Finally, our main HTTP request handler. Here we just call the `parse_url_and_apply` routine, and then queue up the generated content handler that was passed in by `user_ctx`:

```c
static esp_err_t httpd_handle_request(httpd_req_t* req) {
    httpd_work_fn_t handler = (httpd_work_fn_t)req->user_ctx;
    if (handler == NULL) {
        ESP_LOGE(TAG,"No HTTP handler!");
        return ESP_FAIL;
    }
    parse_url_and_apply(req->uri);
    httpd_async_resp_arg_t* resp_arg = (httpd_async_resp_arg_t*)malloc(sizeof(httpd_async_resp_arg_t));
    if (resp_arg == NULL) {
        ESP_LOGE(TAG,"No memory for HTTP request!");
        return ESP_ERR_NO_MEM;
    }
    resp_arg->fd = httpd_req_to_sockfd(req);
    if (resp_arg->fd == -1) {
        free(resp_arg);
        return ESP_FAIL;
    }
    resp_arg->hd = httpd_handle;
    ESP_LOGE(TAG,"Queueing HTTP response");
    return httpd_queue_work(httpd_handle, handler, resp_arg);
}
```
ClASP is the reason the above is so minimal. All the real work is done by the content handlers which are generated as part of `/include/httpd_content.h`. 

Next we have the routine to actually initialize the httpd www server:

```c
static bool www_start(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = HTTPD_RESPONSE_HANDLER_COUNT;
    config.max_open_sockets = CONFIG_LWIP_MAX_SOCKETS-3;
    config.lru_purge_enable = true;
    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering Captive Portal URI handlers");
        if(ESP_OK!=httpd_register_err_handler(server,HTTPD_404_NOT_FOUND,httpd_err_handler)) {
            return httpd_stop(server);
            httpd_handle = NULL;
            return false;
        }
        for (int i = 0; i < HTTPD_RESPONSE_HANDLER_COUNT; ++i) {
            httpd_uri_t uri_config;
            const httpd_response_handler_t* resp = &httpd_response_handlers[i];
            memset(&uri_config, 0, sizeof(uri_config));
            uri_config.handler = httpd_handle_request;
            uri_config.method = HTTP_GET;
            uri_config.uri = resp->path_encoded;
            httpd_work_fn_t fn = resp->handler;
            uri_config.user_ctx = fn;
            if (ESP_OK != httpd_register_uri_handler(server, &uri_config)) {
                ESP_LOGI(TAG, "Failed to register handler");
                httpd_stop(server);
                httpd_handle = NULL;
                return false;
            }
        }
        
        httpd_handle = server;
        return true;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return false;
}
```
`HTTPD_RESPONSE_HANDLER_COUNT` is provided by ClASP, as is the `httpd_response_handlers` array, so it makes configuring the server as simple as looping through the handlers.

The next routine stops the portal service:

```c
void captive_portal_end(void) {
    if(dns_handle) {
        stop_dns_server(dns_handle);
        dns_handle = NULL;
    }
    if(httpd_handle!=NULL) {
        httpd_stop(httpd_handle);
        httpd_handle = NULL;
    }
    esp_netif_dhcpc_stop(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"));
    wifi_deinit_softap();
    esp_netif_destroy_default_wifi(wifi_ap);
    nvs_flash_deinit();
    esp_event_loop_delete_default();
    esp_netif_deinit();
    if(captive_portal_uri!=NULL) {
        free(captive_portal_uri);
        captive_portal_uri = NULL;
    }
}
```

These functions set the callbacks for when a connection is made to the AP or when a connection terminated:

```c
static captive_portal_callback_t captive_portal_on_sta_connect_fn = NULL;
static void* captive_portal_on_sta_connect_state = NULL;
void captive_portal_on_sta_connect(captive_portal_callback_t callback, void* state) {
    captive_portal_on_sta_connect_fn = callback;
    captive_portal_on_sta_connect_state = state;
}
static captive_portal_callback_t captive_portal_on_sta_disconnect_fn = NULL;
static void* captive_portal_on_sta_disconnect_state = NULL;
void captive_portal_on_sta_disconnect(captive_portal_callback_t callback, void* state) {
    captive_portal_on_sta_disconnect_fn = callback;
    captive_portal_on_sta_disconnect_state = state;
}
```

The following monitors the DHCP server for connections and disconnections in order to fire the above callbacks:

```c
static wifi_sta_list_t dhcp_sta_list;
void dhcp_monitor(void* arg) {
    bool connected = false;
    while(dns_handle!=NULL) {
        vTaskDelay(5);
        if(ESP_OK==esp_wifi_ap_get_sta_list(&dhcp_sta_list)) {
            if(dhcp_sta_list.num>0) {
                if(!connected) {
                    connected = true;
                    if(captive_portal_on_sta_connect_fn!=NULL) {
                        captive_portal_on_sta_connect_fn(captive_portal_on_sta_connect_state);
                    }
                }
            } else {
                if(connected) {
                    connected = false;
                    if(captive_portal_on_sta_disconnect_fn!=NULL) {
                        captive_portal_on_sta_disconnect_fn(captive_portal_on_sta_disconnect_state);
                    }
                }
            }
        }
    }
    vTaskDelete(NULL);
}
```

`http_send_block()` is the core method for transfering data over the HTTP transport socket. It is used by ClASP generated response handlers to emit content:

```c
static void httpd_send_block(const char* data, size_t len, void* arg) {
    if (!data || !len) {
        return;
    }
    httpd_async_resp_arg_t* resp_arg = (httpd_async_resp_arg_t*)arg;
    int fd = resp_arg->fd;
    httpd_handle_t hd = (httpd_handle_t)resp_arg->hd;
    httpd_socket_send(hd, fd, data, len, 0);
}
```

The next method is used to send HTTP chunked encoded data over the HTTP transport:

```c
static void httpd_send_chunked(const char* buffer, size_t buffer_len,
                        void* arg) {
    char buf[64];
    if (buffer && buffer_len) {
        itoa(buffer_len, buf, 16);
        strcat(buf, "\r\n");
        httpd_send_block(buf, strlen(buf), arg);
        httpd_send_block(buffer, buffer_len, arg);
        httpd_send_block("\r\n", 2, arg);
        return;
    }
    httpd_send_block("0\r\n\r\n", 5, arg);
}
```

Now we have the expression method used by ClASP for handling `<%= =>` blocks:

```c
static void httpd_send_expr(const char* expr, void* arg) {
    if (!expr || !*expr) {
        return;
    }
    httpd_send_chunked(expr, strlen(expr), arg);
}
```

Next up is the method we use in our request handler to crack the query string into name/value pairs:

```c
static const char* httpd_crack_query(const char* url_part, char* name,
                                     char* value) {
    if (url_part == NULL || !*url_part) return NULL;
    const char start = *url_part;
    if (start == '&' || start == '?') {
        ++url_part;
    }
    size_t i = 0;
    char* name_cur = name;
    while (*url_part && *url_part != '=' && *url_part != '&') {
        if (i < 64) {
            *name_cur++ = *url_part;
        }
        ++url_part;
        ++i;
    }
    *name_cur = '\0';
    if (!*url_part || *url_part == '&') {
        *value = '\0';
        return url_part;
    }
    ++url_part;
    i = 0;
    char* value_cur = value;
    while (*url_part && *url_part != '&') {
        if (i < 64) {
            *value_cur++ = *url_part;
        }
        ++url_part;
        ++i;
    }
    *value_cur = '\0';
    return url_part;
}
```

The following decodes an URL encoded string:
```c
static char* httpd_url_decode(char* dst, size_t dstlen, const char* src) {
    char a, b;
    while (*src && dstlen) {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a') a -= 'a' - 'A';
            if (a >= 'A')
                a -= ('A' - 10);
            else
                a -= '0';
            if (b >= 'a') b -= 'a' - 'A';
            if (b >= 'A')
                b -= ('A' - 10);
            else
                b -= '0';
            *dst++ = 16 * a + b;
            dstlen--;
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            dstlen--;
            src++;
        } else {
            *dst++ = *src++;
            dstlen--;
        }
    }
    if (dstlen) {
        *dst++ = '\0';
    }
    return dst;
}
```
The following three methods get our portal's website URL, the credentials of the access point, and the AP address suitable for setting a QR code to, respectively:

```c
bool captive_portal_get_address(char* out_address,size_t out_address_length) {
    if(dns_handle==NULL) {
        return false;
    }
    strncpy(out_address,captive_portal_uri,out_address_length);
    return true;
}

bool captive_portal_get_credentials(char* out_ssid,size_t out_ssid_length, char* out_pass,size_t out_pass_length) {
    if(dns_handle==NULL) {
        return false;
    }
    strncpy(out_ssid,wifi_ssid,out_ssid_length);
    strncpy(out_pass,wifi_pass,out_pass_length);
    return true;
}
bool captive_portal_get_ap_address(char* out_address,size_t out_address_length) {
    if(dns_handle==NULL) {
        return false;
    }
    // https://www.wi-fi.org/system/files/WPA3%20Specification%20v3.5.pdf (page 37)
    //WIFI:T:WPA;S:<SSID>;P:<PASS>;;
    strcpy(out_address, "WIFI:T:WPA;S:");
    strcat(out_address, wifi_ssid);
    strcat(out_address, ";P:");
    strcat(out_address, wifi_pass);
    strcat(out_address, ";;");
    return true;
}
```

That wraps up the configuration portal's supporting code, but let's segue into the `/www` folder in order to better understand `/include/httpd_content.h` and the handlers:

We won't touch `/www/default.css` because it's just static content, except to say it gets compressed and then gets embedded as part of the `httpd_content_default_css()` generated handler.

#### index.clasp

`/www/index.clasp` on the other hand, is far more interesting. I will be covering parts of it, as the entire thing is quite long due to the timezone information. ClASP pages work like classic Microsoft ASP except instead of VBScript or JScript in the backing code, it's C or C++. `<% %>` and `<%= %>` context switch from HTML or otherwise textual content to C/++ code:

```asp
<%@status code="200" text="OK"%>
<%@header name="Content-Type" value="text/html; charset=utf-8"%><%
```
These two lines establish the HTTP response status line and any headers to emit aside from the generated Content-Encoding or Content-Length header - in this case `200 OK` and the `Content-Type` header. Finally there is a `<%` which indicates a context switch into the following C code:

```c
char ssid[65];
char pass[129];
ssid[0]='\0';
pass[0]='\0';
config_get_value("wifi",0,ssid,sizeof(ssid)-1);
config_get_value("wifi",1,pass,sizeof(pass)-1);
```

Here we simply populate the `ssid` and `pass` variables with the contents of the "wifi" configuration values. We use them later.

After that we context switch back to HTML, emit the `DOCTYPE` and a bunch of HTML that follows (omitted here):

```asp
%><!DOCTYPE html>
<html lang="en">
    <head>
        <meta charset="utf-8">
        ...
```

Most of this is just HTML, but there are some context switches embedded in it, like for populating SSID and Password `textbox` inputs:

```
value="<%=ssid%>" 
value="<%=pass%>"
```

Aside from that, it's just a `<form>` with several query parameters passed as HTTP GET. When we receive a request with the `configure` query parameter, we know to set the configuration values and reset the ESP32.

That covers the important bits of the `/www` folder.

That wraps up the code that we'll cover in the article, because the rest is fairly easy to navigate, and I am not out to create a War and Peace length missive on this code, nor force you dear reader, to endure such a thing.

## Conclusion

Creating a cute little retro clock display is nice.
Using a portal to configure an ESP32 to allow it to operate is even better.
I hope you enjoyed this code!