#include <M5EPD.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <FS.h>
#include <LITTLEFS.h>
#include <ezTime.h>
#include <regex>
#include "M5PanelWidget.h"
#include "defs.h"

#define ERR_WIFI_NOT_CONNECTED  "ERROR: Wifi not connected"
#define ERR_HTTP_ERROR          "ERROR: HTTP code "
#define ERR_GETITEMSTATE        "ERROR in getItemState"

#define DEBUG                   true

#define WIDGET_COUNT            6
#define FONT_CACHE_SIZE         256

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

int previousSysInfoMillis = 0;
int currentSysInfoMillis;

int previousRefreshMillis = 0;
int currentRefreshMillis;

uint16_t _last_pos_x = 0xFFFF, _last_pos_y = 0xFFFF;

Timezone openhabTZ;

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

void debug(String function, String message)
{
    if ( DEBUG )
    {
        Serial.print(F("DEBUG (function "));
        Serial.print(function);
        Serial.print(F("): "));
        Serial.println(message);
    }
}

bool httpRequest(String &url,String &response)
{
    HTTPClient http;
    debug(F("httpRequest"),"HTTP request to " + String(url));
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println(ERR_WIFI_NOT_CONNECTED);
        response = String(ERR_WIFI_NOT_CONNECTED);
        return false;
    }
    http.useHTTP10(true);
    http.setReuse(false);
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK)
    {
        Serial.println(String(ERR_HTTP_ERROR) + String(httpCode));
        response = String(ERR_HTTP_ERROR) + String(httpCode);
        http.end();
        return false;        
    }
    response = http.getString();
    http.end();
    debug(F("httpRequest"),F("HTTP request done"));
    return true;
}

void postWidgetValue(const String &itemName, const String &newValue)
{
    HTTPClient httpPost;
    httpPost.setReuse(false);
    httpPost.begin(restUrl + "/items/" + itemName);
    httpPost.addHeader(F("Content-Type"),F("text/plain"));
    httpPost.POST(newValue);
    //httpPost.end();           // to be fixed, http client should be ended, but it also closes subscription client
}

bool subscribe(){
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
    debug(F("subscribe"), "Full subscriptionURL: " + subscriptionURL);
    subscriptionURL = subscriptionURL.substring(subscriptionURL.indexOf("/rest/sitemaps")) + "?sitemap=" + OPENHAB_SITEMAP + "&pageid=" + OPENHAB_SITEMAP; // Fix : pageId
    debug(F("subscribe"), "subscriptionURL: " + subscriptionURL);
    SubscribeClient.connect(OPENHAB_HOST,OPENHAB_PORT);
    SubscribeClient.println("GET " + subscriptionURL + " HTTP/1.1");
    SubscribeClient.println("Host: " + String(OPENHAB_HOST) + ":" + String(OPENHAB_PORT) );
    SubscribeClient.println(F("Accept: text/event-stream"));
    SubscribeClient.println(F("Connection: keep-alive"));
    SubscribeClient.println();
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
    debug(F("updateSiteMap"),"1:"+String(ESP.getFreeHeap()));
    String sitemapStr;
    httpRequest(restUrl + "/sitemaps/" + OPENHAB_SITEMAP, sitemapStr);
    debug(F("updateSiteMap"),"2:"+String(ESP.getFreeHeap()));
    deserializeJson(sitemap, sitemapStr);
    debug(F("updateSiteMap"),"3:"+String(ESP.getFreeHeap()));
    for(byte i = 0; i < 6; i++) { //6
        if (! sitemap["homepage"]["widgets"][i]["type"].isNull())
        {
        String type = sitemap["homepage"]["widgets"][i]["type"];
        String slabel = sitemap["homepage"]["widgets"][i]["label"];
        String label = "";
        String state = "";
        parseWidgetLabel(slabel, label, state);
        String icon = sitemap["homepage"]["widgets"][i]["icon"];
        String itemState = sitemap["homepage"]["widgets"][i]["item"]["state"];
        String itemName = sitemap["homepage"]["widgets"][i]["item"]["name"];
        String itemType = sitemap["homepage"]["widgets"][i]["item"]["type"];
        Serial.println("Item " + String(i) + " label=" + label + " type="+ type + " icon=" + icon + " state=" + state + " itemName=" + itemName + " itemType=" + itemType +" itemState=" + itemState );
        widgets[i].update(label, state, itemState, icon, type, itemName, itemType);
        widgets[i].draw(UPDATE_MODE_GC16); //  UPDATE_MODE_GL16
        }
    else
    {
         widgets[i].clear();
    }
        
    debug(F("updateSiteMap"),"4:"+String(ESP.getFreeHeap()));
    }
    sitemap.clear();
    debug(F("updateSiteMap"),"5:"+String(ESP.getFreeHeap()));
}

void parseSubscriptionData(String jsonDataStr)
{
    DynamicJsonDocument jsonData(30000);
    deserializeJson(jsonData, jsonDataStr);
    if (! jsonData["widgetId"].isNull() ) // Data Widget (subscription)
    {
        debug(F("parseSubscriptionData"),F("Data Widget (subscription)"));
        byte widgetId = jsonData["widgetId"];
        String slabel = jsonData["label"];
        String itemState = jsonData["item"]["state"];
        String itemName = jsonData["item"]["name"];
        String itemType = jsonData["item"]["type"];
        String label = "";
        String state = "";
        parseWidgetLabel(slabel, label, state);
        Serial.println("Update Item " + String(widgetId) + " label=" + label + " state=" + state);
        widgets[widgetId].update(label, state, itemState, itemName, itemType);
        widgets[widgetId].draw(UPDATE_MODE_GC16); // UPDATE_MODE_A2 
    }
    else if (! jsonData["TYPE"].isNull() )
    {
        String jsonDataType = jsonData["TYPE"];
        if ( jsonDataType.equals("ALIVE") )
        {
            debug(F("parseSubscriptionData"),F("Subscription Alive"));
        }
        else if ( jsonDataType.equals("SITEMAP_CHANGED") )
        {
            debug(F("parseSubscriptionData"),F("Sitemap changed, reloading"));
            updateSiteMap();
        }
    }
    jsonData.clear();
}

void setTimeZone() // Gets timezone from OpenHAB
{
    String response;
    if (httpRequest(restUrl + "/services/org.eclipse.smarthome.i18n/config",response))
    {
        //DynamicJsonDocument doc(2000);
        deserializeJson(jsonDoc, response);
        String timezone = jsonDoc["timezone"];
        debug("setTimeZone","OpenHAB timezone= " + timezone);
        jsonDoc.clear();
        openhabTZ.setLocation(timezone);
    }
    else
    {
        debug("setTimeZone","Could not get OpenHAB timezone");
    }
}

void syncRTC()
{
    /*
    RTCtime.hour = now.hour
    RTCtime.min  = timeClient.getMinutes();
    RTCtime.sec  = timeClient.getSeconds();
    M5.RTC.setTime(&RTCtime);

    RTCDate.year = timeClient.getDay();
    RTCDate.mon  = timeClient.getHours();
    RTCDate.day  = timeClient.getDay();
    M5.RTC.setDate(&RTCDate);
*/
    /*
    String dateTime = getItemState(OPENHAB_DATETIME_ITEM);
    RTCtime.hour = dateTime.substring(11,13).toInt();    
    RTCtime.min  = dateTime.substring(14,16).toInt();
    RTCtime.sec  = dateTime.substring(17,19).toInt();
    debug("syncRTC","Time="+String(RTCtime.hour)+":"+String(RTCtime.min)+":"+String(RTCtime.sec));
    M5.RTC.setTime(&RTCtime);

    RTCDate.year = dateTime.substring(0,4).toInt();
    RTCDate.mon  = dateTime.substring(5,7).toInt();
    RTCDate.day  = dateTime.substring(8,10).toInt();
    debug("syncRTC","Date="+String(RTCDate.year)+"-"+String(RTCDate.mon)+"-"+String(RTCDate.day));
    M5.RTC.setDate(&RTCDate);
    */

}

void displaySysInfo()
{
    // Display system information
    canvas.setTextSize(FONT_SIZE_STATUS_CENTER); 
    canvas.setTextDatum(TC_DATUM);

    /*M5.RTC.getTime(&RTCtime);
    char time[6];
    snprintf(time,sizeof(time),"%02d:%02d",RTCtime.hour,RTCtime.min);
    canvas.drawString(time,80,30);*/

    canvas.drawString(openhabTZ.dateTime("H:i"), 80, 40);

    canvas.setTextSize(FONT_SIZE_LABEL); 
    canvas.setTextDatum(TL_DATUM);

    canvas.drawString("Free Heap:",0,250);
    canvas.drawString(String(ESP.getFreeHeap())+ " B",0,290);

    canvas.drawString("Voltage: ",0,340);
    canvas.drawString(String(M5.getBatteryVoltage())+ " mV",0,380);

    upTime = millis()/(60000);
    canvas.drawString("Uptime: ",0,430);
    canvas.drawString(String(upTime)+ " min",0,470);
    canvas.pushCanvas(780,0,UPDATE_MODE_A2);
}

void setup()
{
    M5.begin(true,false,true,true,false); // bool touchEnable = true, bool SDEnable = false, bool SerialEnable = true, bool BatteryADCEnable = true, bool I2CEnable = false
    M5.disableEXTPower(); 
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

    Serial.println(F("Inizializing LITTLEFS FS..."));
    if (LITTLEFS.begin()){
        Serial.println(F("LITTLEFS mounted correctly."));
    }else{
        Serial.println(F("!An error occurred during LITTLEFS mounting"));
    }

    // Get all information of LITTLEFS
    unsigned int totalBytes = LITTLEFS.totalBytes();
    unsigned int usedBytes = LITTLEFS.usedBytes();
 
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
    canvas.loadFont("/FreeSansBold.ttf", LITTLEFS);
    // TODO : Should fail and stop if font not found

    canvas.setTextSize(FONT_SIZE_LABEL);
    canvas.createRender(FONT_SIZE_LABEL,FONT_CACHE_SIZE);
    canvas.createRender(FONT_SIZE_STATUS_CENTER,FONT_CACHE_SIZE);
    canvas.createRender(FONT_SIZE_STATUS_BOTTOM,FONT_CACHE_SIZE);

    // Setup Wifi
    Serial.println(F("Starting Wifi"));
    WiFi.begin(WIFI_SSID, WIFI_PSK);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println(F("WiFi connected"));
    Serial.println(F("IP address: "));
    Serial.println(WiFi.localIP());

    // Init widgets
    for(byte i = 0; i < WIDGET_COUNT; i++) {
        int x = i % BUTTONS_X;
        int y = i / BUTTONS_X;
        widgets[i].init(i, 0, 40 + x * (40 + BUTTON_SIZE), 40 + y * (40 + BUTTON_SIZE));
    }
    // NTP stuff
    setInterval(3600);
    waitForSync();
    setTimeZone();

    displaySysInfo();
    subscribe();
    updateSiteMap();
}

// Loop
void loop()
{
    // Subscribe or re-subscribe to sitemap
    if (! SubscribeClient.connected()) {
        Serial.println(F("SubscribeClient not connected, connecting..."));
        if (! subscribe()) { delay(300); }
    }

    // Check and get subscription data    
    while (SubscribeClient.available()) {
        String SubscriptionReceivedData = SubscribeClient.readStringUntil('\n');
        int dataStart = SubscriptionReceivedData.indexOf("data: ");
        if (dataStart > -1) // received data contains "data: "
        {
            String SubscriptionData = SubscriptionReceivedData.substring(dataStart+6); // Remove chars before "data: "
            int dataEnd = SubscriptionData.indexOf("\n\n");
            SubscriptionData = SubscriptionData.substring(0,dataEnd);
            parseSubscriptionData(SubscriptionData);
        }   
    }
    
    // Check touch
        if (M5.TP.available())
        {
            M5.TP.update();
            bool is_finger_up = M5.TP.isFingerUp();
            if(is_finger_up || (_last_pos_x != M5.TP.readFingerX(0)) || (_last_pos_y != M5.TP.readFingerY(0)))
            {
                _last_pos_x = M5.TP.readFingerX(0);
                _last_pos_y = M5.TP.readFingerY(0);
                if(! is_finger_up)
                {
                    for(byte i = 0; i < WIDGET_COUNT; i++)
                    if (widgets[i].testIfTouched(_last_pos_x,_last_pos_y))
                    {
                        //debug("loop","Widget touched: " + String(i));
                        String itemName;
                        String newValue;
                        widgets[i].getTouchedValues(itemName, newValue);
                        //debug("loop","Touched values: " + itemName +", " + newValue);
                        if (! newValue.isEmpty()) 
                        {
                            widgets[i].drawPushedBorder(UPDATE_MODE_A2);
                            postWidgetValue(itemName,newValue);
                            //debug("loop","POST values: " + itemName +", " + newValue);
                        }
                    }
                }
            }
            M5.TP.flush();
        }

    // Display sysinfo
    if ( DISPLAY_SYSINFO ) {
        currentSysInfoMillis = millis();
        if ( (currentSysInfoMillis-previousSysInfoMillis) > 10000) 
        {
            previousSysInfoMillis = currentSysInfoMillis;
            displaySysInfo();
            //debug("loop","Total PSRAM: %d" + String(ESP.getPsramSize()));
            //debug("loop","Free PSRAM: %d" + String(ESP.getFreePsram()));
        }
    }

    // Full refresh every 10 minutes to clear artefacts
    currentRefreshMillis = millis();
    if ((currentRefreshMillis-previousRefreshMillis) > 600000 ) // 600000
    {
        previousRefreshMillis = currentRefreshMillis;
        M5.EPD.UpdateFull(UPDATE_MODE_GL16);
        debug("Loop","Full refresh");
    }
    events(); // for ezTime
}