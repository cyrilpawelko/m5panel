#include <Arduino.h>
#include <M5EPD.h>

#define BUTTONS_X               3       // bttons columns
#define BUTTONS_Y               2       // buttons rows
#define BUTTON_SIZE             210     // button widht and height

#define FONT_SIZE_LABEL         32 
#define FONT_SIZE_STATUS_CENTER 48
#define FONT_SIZE_STATUS_BOTTOM 36



class M5PanelWidget
{
    private:
        byte index;
        byte page;  // always 0
        int xLeftCorner;
        int yLeftCorner;
        String title;
        String value;
        String icon;
        String type;
        String itemName;
        String itemType;
        String itemState;
        M5EPD_Canvas* canvas;
    public:
        void init(byte index, byte page, int xLeftCorner, int yLeftCorner);
        void update(const String &title, const String &value, const String &itemState, const String &icon, const String &type, const String &itemName, const String &itemType);
        void update(const String &title, const String &value, const String &itemState, const String &itemName, const String &itemType);
        //void update(const String &value);
        void draw(m5epd_update_mode_t drawMode);
        bool testIfTouched(uint16_t x, uint16_t y);
        void getTouchedValues(String &item, String &newValue);
};

/* Docs:
    https://github.com/m5stack/M5Paper_FactoryTest/blob/main/src/epdgui/epdgui_button.h
    https://github.com/m5stack/M5Paper_FactoryTest/blob/main/src/epdgui/epdgui_button.cpp
*/