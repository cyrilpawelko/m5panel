#include <M5EPD.h>
#include <WiFi.h>
#include "defs.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>

M5EPD_Canvas canvas(&M5.EPD);
String restUrl = "http://" + String(OPENHAB_HOST) + String(":") +String(OPENHAB_PORT) + String("/rest");
unsigned long upTime;

#define ERR_WIFI_NOT_CONNECTED  "ERROR: Wifi not connected"
#define ERR_HTTP_ERROR          "ERROR: HTTP code "
#define ERR_GETITEMSTATE        "ERROR in getItemState"

#define BUTTONS_X               3       // bttons columns
#define BUTTONS_Y               2       // buttons rows
#define BUTTON_SIZE             210     // button widht and height

boolean httpRequest(String &_url,String &_response)
{
    HTTPClient http;
    Serial.println("HTTP request to " + String(_url));
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println(ERR_WIFI_NOT_CONNECTED);
        _response = String(ERR_WIFI_NOT_CONNECTED);
        return false;
    }
    http.useHTTP10(true);
    http.begin(_url);
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK)
    {
        Serial.println(String(ERR_HTTP_ERROR) + String(httpCode));
        _response = String(ERR_HTTP_ERROR) + String(httpCode);
        http.end();
        return false;        
    }
    _response = http.getString();
    http.end();
    Serial.print("HTTP request done");
    return true;
}

String getItemState(String _item)
{
    String response;
    Serial.println(String("getItemState ") + _item);
    if (httpRequest(restUrl + "/items/" + _item,response))
    {
        DynamicJsonDocument doc(65536);
        deserializeJson(doc, response);
        String itemState = doc["state"];
        String itemName = doc["name"];
        Serial.println(itemName + String(":") + itemState);
        doc.clear();
        return itemState;
    }
    else
    {
        return ERR_GETITEMSTATE;
    }
}

boolean getItem(String &_item, String &_state, String &_type, String &_pattern, String &_label )
{
    String response;
    Serial.println(String("getItemState ") + _item);
    if (httpRequest(restUrl + "/items/" + _item,response))
    {
        DynamicJsonDocument doc(65536);
        deserializeJson(doc, response);
        _state   = doc["state"].as<String>();
        _type    = doc["type"].as<String>();
        _pattern = doc["statedescription"]["pattern"].as<String>();
        _label   = doc["label"].as<String>();
        doc.clear();
        return true;
    }
    else
    {
        return false;
    }
}

void button(byte _x, byte _y, const String &_title, const String &_value)
{
    // Remplir le canvas avec du vide
    int xTextAdjust;
    // DRAW RECTANGLE
    int xLeftCorner = 210 + _x * (40 + BUTTON_SIZE);
    int yLeftCorner =  40 + _y * (40 + BUTTON_SIZE);
    canvas.fillRoundRect(xLeftCorner-2,yLeftCorner-2,BUTTON_SIZE+4,BUTTON_SIZE+4,5,15);
    canvas.fillRoundRect(xLeftCorner,yLeftCorner,BUTTON_SIZE,BUTTON_SIZE,5,0);
    
    // DRAW TITLE
    canvas.setTextSize(2);
    xTextAdjust = (BUTTON_SIZE - canvas.textWidth(_title))/2;
    canvas.drawString(_title,xLeftCorner+xTextAdjust,yLeftCorner+20);
    
    // DRAW VALUE
    canvas.setTextSize(4);
    xTextAdjust = (BUTTON_SIZE - canvas.textWidth(_value))/2;
    canvas.drawString(_value,xLeftCorner+xTextAdjust,yLeftCorner+BUTTON_SIZE-80);
}

void setup()
{
    M5.begin();
    //WiFi.config(staticIP, subnet, gateway, dns); 
    //M5.EPD.SetRotation(180);
    M5.EPD.Clear(true);
    M5.RTC.begin();

    canvas.createCanvas(960, 540);
    canvas.setTextSize(3);

// SETUP WIFI
    Serial.println("Starting Wifi");
    WiFi.begin(WIFI_SSID, WIFI_PSK);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

// LOOP
void loop()
{
    for(byte i = 0; i < 6; i++) {
        String item = openhabItems[i];
        if (item != "") {
            int x = i % BUTTONS_X;
            int y = i / BUTTONS_X;
            String state;       // item state
            String type;        // item type
            String pattern;     // item pattern
            String label;       // item label
            getItem(item, state, type, pattern, label);
            button(x, y, label, state);
        }
    }
    canvas.pushCanvas(0,0,UPDATE_MODE_GL16); 
    delay(REFRESH_INTERVAL * 1000);
}