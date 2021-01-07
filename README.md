# m5Panel for OpenHAB
![m5paper](doc/m5panel.jpg)

## Introduction

This is a preliminary release of using a [m5paper](https://m5stack.com/products/m5paper-esp32-development-kit-960x540-4-7-eink-display-235-ppi) as an automation panel for OpenHAB.

I want it to be as simple as possible : it queries OpenHAB items through REST API, so much of the configuration will be on the OpenHAB side. I don't want "yet another interface to configure with its cryptic syntax".

The advanced configuration mode still has to be defined (probably through a custom sitemap).
  
Actually, it just displays the 6 specified OpenHAB item's Label and Status, and refreshes every twenty seconds. 
While the is no power optimizations, it can already run several hours on battery.

It has been tested with OpenHAB 2.5 and 3.0

## How to
 - Clone and open in PlatformIO
 - Copy defs-sample.h to defs.h
 - Edit defs.h and customize:
    - Wifi settings
    - Openhab host and port
    - Items to be displayed (as in OpenHAB configuration)
- Upload to m5paper
- Monitor through serial port

No special configuration is needed on the OpenHAB site. Just check you can reach REST API at http://<OPENHAB_HOST>:<OPENHAB_PORT>/items

## Known issues
 - Incorrect display of special chars and accents (probably due to the basic font used)
 - No touch screen support

## Todo
- [ ] WifiManager for Wifi and items setup
- [ ] Provide binary releases
- [ ] Fancy font and correct encoding
- [ ] Dynamic updates
- [ ] Fancy widgets (gauge, switch, garagedoor, ...)
- [ ] Advanced widgets (weather, ...)
- [ ] Touch screen support for commands (switchs, ...)
- [ ] Multi-page navigation
- [ ] Advanced configuration method (for widgets, ...)
