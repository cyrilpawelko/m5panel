#include "M5PanelWidget.h"

void M5PanelWidget::init(byte index, byte page, int xLeftCorner, int yLeftCorner)
        {
            this->index = index;
            this->page = page;
            this->xLeftCorner = xLeftCorner;
            this->yLeftCorner = yLeftCorner;
            this->type = type;
            this->canvas = new M5EPD_Canvas(&M5.EPD);
            this->canvas->createCanvas(BUTTON_SIZE+4, BUTTON_SIZE+4);
            Serial.print("M5PanelWidget::init index=");
            Serial.println(this->index);            
        }

void  M5PanelWidget::update(const String &title, const String &value, const String &itemState, const String &icon, const String &type)
{
    this->title = title;
    this->value = value;
    this->itemState = itemState;
    this->icon = icon;
    this->type = type;
}

void M5PanelWidget::update(const String &title, const String &value, const String &itemState)
{
    this->title = title;
    this->value = value;
    this->itemState = itemState;
    this->icon = icon;
}

 void M5PanelWidget::testIfTouched(uint16_t x, uint16_t y)
 {
    if ((x > xLeftCorner) && (x < (this->xLeftCorner+BUTTON_SIZE)) && (y > yLeftCorner) && (y < (yLeftCorner+BUTTON_SIZE)))
    {
        Serial.println("Widget touched:" + String(index));
        // Send command to item
        if (type="Switch")
        {
            // post different state
        }
    }
}

void M5PanelWidget::draw(m5epd_update_mode_t drawMode)
{
    long startTime = millis();
    this->canvas->fillRoundRect(0, 0, BUTTON_SIZE+4, BUTTON_SIZE+4, 5, 15);            
    this->canvas->fillRoundRect(2, 2, BUTTON_SIZE, BUTTON_SIZE, 5, 0);

    // Draw title
    this->canvas->setTextSize(FONT_SIZE_LABEL);
    this->canvas->setTextDatum(TC_DATUM);
    this->canvas->drawString(title,BUTTON_SIZE/2+2,15+2);
    
    long titleTime = millis();
    long iconTime;
    long bottomTime;

    if ( this->icon == "") 
    {   
        // Draw value centered
        this->canvas->setTextSize(FONT_SIZE_STATUS_CENTER);
        this->canvas->setTextDatum(TC_DATUM);
        this->canvas->drawString(value,BUTTON_SIZE/2+2,BUTTON_SIZE-120+2);
        iconTime = millis();
        bottomTime = iconTime;
    }
    else
    {
        String iconFile = "/icons/"+icon+"-"+value+".png"; // Try to find dynamic icon ...
        iconFile.toLowerCase();
        if (! SPIFFS.exists(iconFile))
        {
            iconFile = "/icons/"+icon+".png";              // else try to find non dynamic icon
            iconFile.toLowerCase();
            if (! SPIFFS.exists(iconFile))
            {
                iconFile = "/icons/unknown.png";      // else use icon "unknown"
            }
        }
        //Serial.println("icon file="+iconFile);
        this->canvas->drawPngFile(SPIFFS,iconFile.c_str(),BUTTON_SIZE/2-48+2,55+2,0,0,0,0,1);
        
        iconTime = millis();

        // Draw bottom value
        this->canvas->setTextSize(FONT_SIZE_STATUS_BOTTOM); 
        this->canvas->setTextDatum(TC_DATUM);
        this->canvas->drawString(value,BUTTON_SIZE/2+2,BUTTON_SIZE-45+2); // 40
        bottomTime = millis();
    }

    // Push canvas
    this->canvas->pushCanvas(xLeftCorner, yLeftCorner, drawMode); // UPDATE_MODE_GL16 (higher quality)   or UPDATE_MODE_A2 (no flipping)
    long canvasTime = millis();
    Serial.println("Draw stats (ms): title=" + String(titleTime-startTime) +  " icon=" + String(iconTime - titleTime) + " bottom=" + String(bottomTime-iconTime)+ " canvas=" + String(canvasTime-bottomTime));
}
