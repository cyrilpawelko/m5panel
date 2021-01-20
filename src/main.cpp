#include <M5EPD.h>
#include <WiFi.h>
#include "defs.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <FS.h>

#define ERR_WIFI_NOT_CONNECTED  "ERROR: Wifi not connected"
#define ERR_HTTP_ERROR          "ERROR: HTTP code "
#define ERR_GETITEMSTATE        "ERROR in getItemState"

#define BUTTONS_X               3       // bttons columns
#define BUTTONS_Y               2       // buttons rows
#define BUTTON_SIZE             210     // button widht and height

#define FONT_SIZE_LABEL         32 //30
#define FONT_SIZE_STATUS_CENTER 56
#define FONT_SIZE_STATUS_BOTTOM 32

M5EPD_Canvas canvas(&M5.EPD);
String restUrl = "http://" + String(OPENHAB_HOST) + String(":") +String(OPENHAB_PORT) + String("/rest");
String iconURL = "http://" + String(OPENHAB_HOST) + String(":") +String(OPENHAB_PORT) + String("/icon");

unsigned long upTime;

#ifndef OPENHAB_SITEMAP
    #define OPENHAB_SITEMAP "m5paper"
#endif

// m5paper code

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

void button(byte _x, byte _y, const String &_title, const String &_value, const String &_icon)
{
    // Draw rectangle
    int xLeftCorner = 40 + _x * (40 + BUTTON_SIZE); // 210
    int yLeftCorner = 40 + _y * (40 + BUTTON_SIZE);
    canvas.fillRoundRect(xLeftCorner-2,yLeftCorner-2,BUTTON_SIZE+4,BUTTON_SIZE+4,5,15);
    canvas.fillRoundRect(xLeftCorner,yLeftCorner,BUTTON_SIZE,BUTTON_SIZE,5,0);

    // Draw title
    canvas.setTextSize(FONT_SIZE_LABEL); //3
    canvas.setTextDatum(TC_DATUM);
    canvas.drawString(_title,xLeftCorner+BUTTON_SIZE/2,yLeftCorner+15);
    
    if ( _icon == "") 
    {   
        // Draw value centered
        canvas.setTextSize(FONT_SIZE_STATUS_CENTER);
        canvas.setTextDatum(TC_DATUM);
        canvas.drawString(_value,xLeftCorner+BUTTON_SIZE/2,yLeftCorner+BUTTON_SIZE-120);
    }
    else
    {
        // Draw icon
        /*String thisIconURL=iconURL+"/"+_icon+"?state="+_value+"&format=png";
        int thisIconURLLenght = thisIconURL.length()+1;
        char cThisIconURL[thisIconURLLenght];
        thisIconURL.toCharArray(cThisIconURL,thisIconURLLenght);
        Serial.println(cThisIconURL);
        canvas.drawPngUrl(cThisIconURL);*/

        String iconFile = "/icons/"+_icon+"-"+_value+".png";
        iconFile.toLowerCase();
        if (! SPIFFS.exists(iconFile))
        {
            iconFile = "/icons/"+_icon+".png";
            iconFile.toLowerCase();
            if (! SPIFFS.exists(iconFile))
            {
                String iconFile = "/icons/unknow.png";
            }
        }
        Serial.println("icon file="+iconFile);
        canvas.drawPngFile(SPIFFS,iconFile.c_str(),xLeftCorner+BUTTON_SIZE/2-48,yLeftCorner+55,0,0,0,0,2);
        
        // Draw bottom value
        canvas.setTextSize(FONT_SIZE_STATUS_BOTTOM); 
        canvas.setTextDatum(TC_DATUM);
        canvas.drawString(_value,xLeftCorner+BUTTON_SIZE/2,yLeftCorner+BUTTON_SIZE-40);
    }
}

void setup()
{
    M5.begin();

/* Uncomment for static IP
    IPAddress ip(192,168,0,xxx);    // Node Static IP
    IPAddress gateway(192,168,0,xxx); // Network Gateway (usually Router IP)
    IPAddress subnet(255,255,255,0);  // Subnet Mask
    IPAddress dns1(xxx,xxx,xxx,xxx);    // DNS1 IP
    IPAddress dns2(xxx,xxx,xxx,xxx);    // DNS2 IP
    WiFi.config(ip, gateway, subnet, dns1, dns2);
*/
    // M5.EPD.SetRotation(180);
    M5.EPD.Clear(true);
    M5.RTC.begin();

    // FS Setup
    Serial.println(F("Inizializing FS..."));
    if (SPIFFS.begin()){
        Serial.println(F("SPIFFS mounted correctly."));
    }else{
        Serial.println(F("!An error occurred during SPIFFS mounting"));
    }

    // Get all information of SPIFFS
 
    unsigned int totalBytes = SPIFFS.totalBytes();
    unsigned int usedBytes = SPIFFS.usedBytes();
 
    // TODO : Should fail and stop if SPIFFS error

    Serial.println("===== File system info =====");
 
    Serial.print("Total space:      ");
    Serial.print(totalBytes);
    Serial.println("byte");
 
    Serial.print("Total space used: ");
    Serial.print(usedBytes);
    Serial.println("byte");
 
    Serial.println();

    canvas.createCanvas(960, 540);
    canvas.loadFont("/FreeSansBold.ttf", SPIFFS);
    // TODO : Should fail and stop if font not found
    canvas.setTextSize(FONT_SIZE_LABEL);
    canvas.createRender(FONT_SIZE_LABEL,256);
    canvas.createRender(FONT_SIZE_STATUS_CENTER,256);
    canvas.createRender(FONT_SIZE_STATUS_BOTTOM,256);

    // Setup Wifi
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

// Loop
void loop()
{
    String sitemapStr;
    httpRequest(restUrl + "/sitemaps/" + OPENHAB_SITEMAP, sitemapStr);
    DynamicJsonDocument sitemap(65536);
    deserializeJson(sitemap, sitemapStr);
    for(byte i = 0; i < 6; i++) {
        if (! sitemap["homepage"]["widgets"][i]["type"].isNull())
        {
        String type = sitemap["homepage"]["widgets"][i]["type"];
        String slabel = sitemap["homepage"]["widgets"][i]["label"];
        String label = "";
        String state;
        int firstOpeningBracket = slabel.indexOf('[');
        int firstClosingBracket = slabel.indexOf(']');

        if ( (firstOpeningBracket == -1) || (firstClosingBracket == -1) ) // Value not found
        {
            slabel.trim();
            label = slabel;
            state = "";
        }
        else
        {
            label = slabel.substring(0,firstOpeningBracket);
            label.trim();
            state = slabel.substring(firstOpeningBracket + 1, firstClosingBracket);
            state.trim();
        }
        
        String icon = sitemap["homepage"]["widgets"][i]["icon"];
        Serial.println("Item " + String(i) + " label=" + label + " type="+ type + " icon=" + icon + " state=" + state);
        
        int x = i % BUTTONS_X;
        int y = i / BUTTONS_X;
        button(x, y, label, state, icon);
        }
    }
    sitemap.clear();
    canvas.pushCanvas(0,0,UPDATE_MODE_GL16); 
    delay(REFRESH_INTERVAL * 1000);
}