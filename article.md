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

