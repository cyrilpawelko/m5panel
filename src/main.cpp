#include <M5EPD.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <FS.h>
#include "M5PanelWidget.h"
#include "defs.h"
#include <regex>

#define ERR_WIFI_NOT_CONNECTED  "ERROR: Wifi not connected"
#define ERR_HTTP_ERROR          "ERROR: HTTP code "
#define ERR_GETITEMSTATE        "ERROR in getItemState"

#define DEBUG                   true

#define WIDGET_COUNT            6
#define FONT_CACHE_SIZE         255 // 256
// Global vars

M5EPD_Canvas canvas(&M5.EPD);
M5PanelWidget* widgets = new M5PanelWidget[WIDGET_COUNT];
HTTPClient httpClient;
HTTPClient httpSubscribeClient;
WiFiClient SubscribeClient;

String restUrl = "http://" + String(OPENHAB_HOST) + String(":") +String(OPENHAB_PORT) + String("/rest");
String iconURL = "http://" + String(OPENHAB_HOST) + String(":") +String(OPENHAB_PORT) + String("/icon");
String subscriptionURL = "";

unsigned long upTime;

DynamicJsonDocument jsonDoc(30000); // size to be checked
DynamicJsonDocument sitemap(30000); // Test

uint previousSysInfoMillis = 0;
uint currentSysInfoMillis;

#ifndef OPENHAB_SITEMAP
    #define OPENHAB_SITEMAP "m5panel"
#endif

#ifndef OPENHAB_SITEMAP
    #define DISPLAY_SYSINFO false
#endif

/* Reminders
    EPD canvas library https://docs.m5stack.com/#/en/api/m5paper/epd_canvas
    Text aligment https://github.com/m5stack/M5Stack/blob/master/examples/Advanced/Display/TFT_Float_Test/TFT_Float_Test.ino
*/

// HTTP and REST

void debug(String message)
{
    if ( DEBUG )
    {
        Serial.println(message);
    }
}

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
    http.setReuse(false);
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
    Serial.println("HTTP request done");
    return true;
}

boolean subscribe(){
    String subscribeResponse;
    httpClient.useHTTP10(true);
    httpClient.begin(restUrl + "/sitemaps/events/subscribe");
    int httpCode = httpClient.POST("");
    if (httpCode != HTTP_CODE_OK)
    {
        Serial.println(String(ERR_HTTP_ERROR) + String(httpCode));
        httpClient.end();
        return false;        
    }
    subscribeResponse = httpClient.getString();
    httpClient.end();

    //Serial.println("HTTP SUBSCRIBE: " + subscribeResponse);
    deserializeJson(jsonDoc,subscribeResponse);

    //String subscriptionURL = jsonDoc["Location"].as<String>();
    String subscriptionURL = jsonDoc["context"]["headers"]["Location"][0];
    debug("Full subscriptionURL: " + subscriptionURL);
    subscriptionURL = subscriptionURL.substring(subscriptionURL.indexOf("/rest/sitemaps")) + "?sitemap=m5panel&pageid=m5panel";
    debug("subscriptionURL: " + subscriptionURL);
        
    //SubscribeClient.connect("192.168.3.11",8080);
    SubscribeClient.connect(OPENHAB_HOST,OPENHAB_PORT);
    SubscribeClient.println("GET " + subscriptionURL + " HTTP/1.1");
    //SubscribeClient.println("Host: nas.pawelko.local:8080");
    SubscribeClient.println("Host: " + String(OPENHAB_HOST) + ":" + String(OPENHAB_PORT) );
    SubscribeClient.println("Accept: text/event-stream");
    SubscribeClient.println("Connection: keep-alive");
    SubscribeClient.println("");
    return true;
}

void parseWidgetLabel(String sitemapLabel, String &label, String &state )
{
    int firstOpeningBracket = sitemapLabel.indexOf('[');
    int firstClosingBracket = sitemapLabel.indexOf(']');
    if ( (firstOpeningBracket == -1) || (firstClosingBracket == -1) ) // Value not found
    {
        sitemapLabel.trim();
        label = sitemapLabel;
        state = "";
    }
    else
    {
        label = sitemapLabel.substring(0,firstOpeningBracket);
        label.trim();
        state = sitemapLabel.substring(firstOpeningBracket + 1, firstClosingBracket);
        state.trim();
    }
}

void updateSiteMap(){
    String sitemapStr;
    httpRequest(restUrl + "/sitemaps/" + OPENHAB_SITEMAP, sitemapStr);
    deserializeJson(sitemap, sitemapStr);
    for(byte i = 0; i < 6; i++) {
        if (! sitemap["homepage"]["widgets"][i]["type"].isNull())
        {
        String type = sitemap["homepage"]["widgets"][i]["type"];
        String slabel = sitemap["homepage"]["widgets"][i]["label"];
        String label = "";
        String state = "";
        parseWidgetLabel(slabel, label, state);
        String icon = sitemap["homepage"]["widgets"][i]["icon"];
        Serial.println("Item " + String(i) + " label=" + label + " type="+ type + " icon=" + icon + " state=" + state);
        widgets[i].update(label, state, icon);
        widgets[i].draw(UPDATE_MODE_GC16); //  UPDATE_MODE_GL16
        }
    }
    sitemap.clear();
}

void parseSubscriptionData(String jsonDataStr)
{
    DynamicJsonDocument jsonData(30000);
    deserializeJson(jsonData, jsonDataStr);
    if (! jsonData["widgetId"].isNull() ) // Data Widget (subscription)
    {
        Serial.println("Data Widget (subscription)");
        byte widgetId = jsonData["widgetId"];
        String slabel = jsonData["label"];
        String label = "";
        String state = "";
        parseWidgetLabel(slabel, label, state);
        Serial.println("Update Item " + String(widgetId) + " label=" + label + " state=" + state);
        widgets[widgetId].update(label, state);
        widgets[widgetId].draw(UPDATE_MODE_GC16); // UPDATE_MODE_A2 
    }
    else if (! jsonData["TYPE"].isNull() )
    {
        String jsonDataType = jsonData["TYPE"];
        if ( jsonDataType.equals("ALIVE") )
        {
            Serial.println("Subscription Alive");
        }
        else if ( jsonDataType.equals("SITEMAP_CHANGED") )
        {
            Serial.println("Sitemap changed, reloading");
            updateSiteMap();
        }
    }
}

void displaySysInfo()
{
    // Show system information
    canvas.setTextSize(FONT_SIZE_STATUS_BOTTOM); 
    canvas.setTextDatum(TL_DATUM);

    canvas.drawString("Free Heap:",0,230);
    canvas.drawString(String(ESP.getFreeHeap())+ " B",0,270);

    canvas.drawString("Voltage: ",0,330);
    canvas.drawString(String(M5.getBatteryVoltage())+ " mV",0,370);

    upTime = millis()/(60000);
    canvas.drawString("Uptime: ",0,430);
    canvas.drawString(String(upTime)+ " min",0,470);
    canvas.pushCanvas(780,0,UPDATE_MODE_A2);
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

    canvas.createCanvas(160, 540);
    canvas.loadFont("/FreeSansBold.ttf", SPIFFS);
    // TODO : Should fail and stop if font not found

    canvas.setTextSize(FONT_SIZE_LABEL);
    canvas.createRender(FONT_SIZE_LABEL,FONT_CACHE_SIZE);
    canvas.createRender(FONT_SIZE_STATUS_CENTER,FONT_CACHE_SIZE);
    canvas.createRender(FONT_SIZE_STATUS_BOTTOM,FONT_CACHE_SIZE);

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

    // Init widgets
    for(byte i = 0; i < WIDGET_COUNT; i++) {
        int x = i % BUTTONS_X;
        int y = i / BUTTONS_X;
        widgets[i].init(i, 0, 40 + x * (40 + BUTTON_SIZE), 40 + y * (40 + BUTTON_SIZE));
    }
    updateSiteMap();
}

// Loop
void loop()
{
    // subscribe or re-subscribe to sitemap
    if (! SubscribeClient.connected()) {
        Serial.println("SubscribeClient not connected, connecting...");
        if (! subscribe()) { delay(300); }
    }

    // Check and get subscription data    
    while (SubscribeClient.available()) {
        String SubscriptionReceivedData = SubscribeClient.readStringUntil('\n');
        int dataStart = SubscriptionReceivedData.indexOf("data: ");
        if (dataStart > -1) // received data contains "data: "
        {
            String SubscriptionData = SubscriptionReceivedData.substring(dataStart+6); // cut data before "data: "
            int dataEnd = SubscriptionData.indexOf("\n\n");
            SubscriptionData = SubscriptionData.substring(0,dataEnd);
            parseSubscriptionData(SubscriptionData);
        }   
    }
    
    if ( DISPLAY_SYSINFO ) {
        currentSysInfoMillis = millis();
        if ( (currentSysInfoMillis-previousSysInfoMillis) > 10000) 
        {
            previousSysInfoMillis = currentSysInfoMillis;
            displaySysInfo();
        }
    }
}