# m5Panel for OpenHAB
[My image](doc/m5panel.jpg)
## Introduction

This is a preliminary release of using an [m5paper](https://m5stack.com/products/m5paper-esp32-development-kit-960x540-4-7-eink-display-235-ppi) as an automation panel for OpenHAB.

I want it to be as simple as possible : It request OpenHAB items through REST API, so much of the configuration will be on the OpenHAB side. I don't want "another interface with its cryptic syntax to configure again".

The advanced configuration mode still has to be defined (probably through a custom sitemap).
  
Actually, it just displays the 6 specified OpenHAB item's Label and Status, and refreshed then every twenty seconds. While the is no power optimizations, it can run several hours on battery.

## How to
 - Clone and open in PlatformIO
 - Edit defs.h and customize
    - Wifi settings
    - Openhab host and port
    - Items to be displayed (as in OpenHAB configuration)
- Upload to m5paper
- Monitor through serial port

## Known issues
 - Incorrect display of special chars and accents (probably due to the basic font used)
 - No touch screen support

## Todo
- [ ] WifiManager for Wifi and items setup
- [ ] Provide binary releases
- [ ] Fancy font and correct encoding
- [ ] Fancy widgets (gauge, switch, garagedoor, ...)
- [ ] Advanced widgets (weather, ...)
- [ ] Touch screen support for commands (switchs, ...)
- [ ] Multi-page navigation
- [ ] Advanced configuration method (for widgets, ...)
