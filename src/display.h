#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include "droplet.h"

//Enumerate menu types
enum MenuType {
  STRETCH,
  WATER,
  BRIGHTNESS
};

//Struct to store menu data
struct Menus {
  MenuType type;
  String name;
  bool isOn; //True: Timer/LED is used | False: Timer/LED not used
  int current; //Currently selected value
  int max_val; //Based in milliseconds/LED brightness
  int increment; //The increment between timer/brightness values
  unsigned long timer; //Starting time
  bool isDefault; //Check if menu is in default mode
};

class ManageDisplays {
    public:
        //Each menu data
        //Max takes into account wrap around / using modulo
        Menus stretch_menu = {STRETCH, "Stretch Break", true, 0, 95, 5, 0, true}; //Max: 90 minutes | Increment: 5 minutes
        Menus water_menu = {WATER, "Water Break", true, 0, 65, 5, 0, true}; //Max: 60 minutes | Increment: 5 minutes
        Menus brightness_menu = {BRIGHTNESS, "Brightness", true, 0, 306, 51, 0, true}; //Max: 255 | Increment: 51

        void start();

        void drawHome(int timer_duration, unsigned long timer_start, bool active);

        void drawMenu(int position);

        void drawReminderSetting(Menus& item);

        void drawLightSetting(Menus& item);

        void drawThermometer(int temp);

        void drawDroplet(int humidity);

        void drawLightBulb(int brightness);

        void drawLeftArrow();

        void drawRightArrow();

        void drawDefaultText();

        String getTime(unsigned long ms);
    
    private:
        TFT_eSPI display = TFT_eSPI();
        TFT_eSprite droplet = TFT_eSprite(&display);

        //Dimentions of droplet sprite
        const int droplet_x = 40;
        const int droplet_y = 50;

        //Colors to represent different bightness
        const uint16_t ORANGE = 0xFC20; //Very bright
        const uint16_t GOLDISH = 0xFDE0; //Bright
        const uint16_t LIGHT_YELLOW = 0xFFE0; //Somewhat bright
        const uint16_t PALE_YELLOW = 0xFFF0; //Not very bright

        const char* menuItems[5] = {"View Temp/Humidity", 
                                    "View Brightness",
                                    "Set Stretch Break",
                                    "Set Water Break",
                                    "Set Brightness"};

        const String normal_face = "O _ O";
        const String rushing_face = "> . <";
        const String alarm_face = "X + X";
        const String no_alarm_face = "z _ z";
};

