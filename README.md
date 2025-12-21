# retro_clock

![Clock screen](https://github.com/user-attachments/assets/50f5bc57-28e3-4b10-8506-20e38ac3a402 "Clock screen")


A retro digital clock that uses NTP to keep time. It is configurable via a portal it exposes when you start it.

An article covering the code is provided [here](https://github.com/codewitch-honey-crisis/retro_clock/blob/main/article.md)

It was primarily designed for the TTGO T1 Display, but I will be adding support for more devices.

## Using

Start the device

It will show you a QR code. Use your phone to connect to the QR provided access point.

The screen will change from "Connect" to "Configure" with a different QR code this time.

Use your phone to navigate to the link.

From there you can set the SSID, password, time zone and 24-hour/12-hour

Once configured the clock will boot into clock mode and attempt to sync.

You can reenter the configuration portal by using either one of the two buttons on the TTGO front panel.

![Connection screen](https://github.com/user-attachments/assets/294ce033-5fee-472e-96ac-c0f0c0a6d9d5 "Connection screen")
![Configuration portal example](https://github.com/user-attachments/assets/f9253b31-eebd-42d8-8650-b98334b02f57?raw=true "Configuration Portal Example")


