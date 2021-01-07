#define WIFI_SSID "mySSID"
#define WIFI_PSK "mySecretPSK"

#define OPENHAB_HOST "openhabian"
#define OPENHAB_PORT 8080

#define REFRESH_INTERVAL 20                 // Refresh interval in seconds

#define maxOpenhabItems 6                   // Do not modify
String openhabItems[maxOpenhabItems] = {    // Specifiy OpenHAB items
    "SonoffTemp01Temperature",              // "" for none
    "SonoffTemp01Humidity",
    "Esp03_Ultrasonic_Volume",
    "gd_currstate",
    "SonoffRelay1Power",
    "SondeTH4_Temperature"
};
