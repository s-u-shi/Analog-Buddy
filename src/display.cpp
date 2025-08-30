#include "display.h"

//Initialize the display
void ManageDisplays::start() {
    display.init();
    display.setRotation(3);
    display.fillScreen(TFT_BLACK);
}

//Print to display the home screen
void ManageDisplays::drawHome(int timer_duration, unsigned long timer_start, bool active) {
    unsigned long now = millis();
    String time_remaining = "--";

    display.fillScreen(TFT_BLACK);
    display.setTextSize(4);
    display.setCursor(60, 60);

    if(!active) {
        //No timers active
        display.print(no_alarm_face);
    }
    else if(now - timer_start >= timer_duration) {
        //Timer is going off
        time_remaining = "0:00:00";
        display.print(alarm_face);
    }
    else {
        unsigned long remaining = timer_duration - (now - timer_start);
        if(remaining <= (timer_duration / 10)) {
            //Getting closer to timer going off
            display.print(rushing_face);
        }
        else {
            //Not that close to timer going off
            display.print(normal_face);
        }
        time_remaining = getTime(remaining);
    }   
    //Display when the closest reminder occurs in
    display.setTextSize(2);
    display.setCursor(10, 10);
    display.print("Reminder: ");
    display.print(time_remaining);

    drawRightArrow();
}

//Print to display the menu
void ManageDisplays::drawMenu(int position) {
    //Clear screen
    display.fillScreen(TFT_BLACK);
    display.setTextSize(1);

    for (int i = 0; i < 5; ++i) {
        int y = 15 + i * 22;

        if (i == position) {
            //If at selected position, add ">" and highlight menu
            display.fillRect(0, y - 10, 240, 25, TFT_DARKGREY);
            display.setTextColor(TFT_WHITE, TFT_DARKGREY);
            display.setCursor(14, y);
            display.print(">");
        } 
        else {
            display.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        
        //Print actual menus
        display.setCursor(30, y);
        display.print(" ");
        display.print(menuItems[i]);
  }

  display.setTextColor(TFT_WHITE, TFT_BLACK);
  drawLeftArrow();
  drawRightArrow();
}

//Print display to change reminder settings
void ManageDisplays::drawReminderSetting(Menus& item) {
    //Title
    display.fillScreen(TFT_BLACK);
    display.setTextSize(2);
    int title_mid = (display.width() - display.textWidth("Set " + item.name)) / 2;
    display.setCursor(title_mid, 20);
    display.print("Set ");
    display.print(item.name);

    //Values
    String value = "";
    if(item.current == 0) {
        value = "OFF";
        item.isOn = false;
    }
    else {
        item.isOn = true;
        value = item.current;
    }
    display.setTextSize(5);
    int value_mid = (display.width() - display.textWidth(String(value))) / 2;
    display.setCursor(value_mid, 65);
    display.print(value);

    drawLeftArrow();
    
    //Display "Default" if value is the default 
    if(item.isDefault) {
        drawDefaultText();
    }
}

//Print display to change light settings
void ManageDisplays::drawLightSetting(Menus& item) {
    //Title
    display.fillScreen(TFT_BLACK);
    display.setTextSize(2);
    int title_mid = (display.width() - display.textWidth("Set " + item.name)) / 2;
    display.setCursor(title_mid, 20);
    display.print("Set ");
    display.print(item.name);

    //Values (lowest to highest)
    display.drawRect(110, 105, 20, 10, TFT_WHITE);
    display.drawRect(102, 91, 36, 10, TFT_WHITE);
    display.drawRect(94, 77, 52, 10, TFT_WHITE);
    display.drawRect(86, 63, 68, 10, TFT_WHITE);
    display.drawRect(78, 49, 84, 10, TFT_WHITE);
    
    //Fill rectangles given brightness
    if(item.current >= 51) {
        display.fillRect(110, 105, 20, 10, TFT_WHITE);
    }
    if(item.current >= 102) {
        display.fillRect(102, 91, 36, 10, TFT_WHITE);
    }
    if(item.current >= 153) {
        display.fillRect(94, 77, 52, 10, TFT_WHITE);
    }
    if(item.current >= 204) {
        display.fillRect(86, 63, 68, 10, TFT_WHITE);
    }
    if(item.current >= 255) {
        display.fillRect(78, 49, 84, 10, TFT_WHITE);
    }

    drawLeftArrow();

    //Display "Default" if value is the default 
    if(item.isDefault) {
        drawDefaultText();
    }
}

//Print to display icon and temp data
void ManageDisplays::drawThermometer(int temp) {
    //Clear Screen
    display.fillScreen(TFT_BLACK);

    //Clear inside of tube
    display.fillRect(50, 5, 30, 76, TFT_BLACK);

    //Bulb - Outline and Inside (always filled)
    display.drawCircle(65, 57, 8, TFT_WHITE);
    display.fillCircle(65, 57, 7, TFT_RED);
    
    //Tube outline
    display.drawRoundRect(60, 5, 10, 50, 5, TFT_WHITE);

    //Map temperature to percentage to fill tube
    int fill = map(temp, 15, 35, 10, 100);
    int fillHeight = (52 * fill) / 100;
    display.fillRoundRect(61, 58 - fillHeight, 8, fillHeight, 5, TFT_RED);

    //Ticks on tube
    for (int i = 1; i <= 4; i++) {
        float fraction = i / float(5);
        int y = 57 - (52 * fraction);
        display.drawFastHLine(66, y, 4, TFT_WHITE);
    }

    //Text Portion
    display.setCursor(105, 10);
    display.setTextSize(2);
    display.print("Temperature");
    display.fillRect(105, 35, 70, 30, TFT_BLACK);
    display.setCursor(105, 35);
    display.setTextSize(3);
    display.print(String(temp) + "C");

    drawLeftArrow();
}

//Print to display the icon and humidity data
void ManageDisplays::drawDroplet(int humidity) {
    int coverage = map(humidity, 0, 100, 50, 0);

    //Fill droplet
    display.fillRect(45, 80, droplet_x, droplet_y, TFT_BLUE);
    display.fillRect(45, 80, droplet_x, coverage, TFT_BLACK);

    //Display sprite/droplet
    droplet.createSprite(droplet_x, droplet_y);
    droplet.setSwapBytes(true);
    droplet.pushImage(0, 0, droplet_x, droplet_y, droplet_icon);
    droplet.pushSprite(45, 80, TFT_WHITE);

    //Text Portion
    display.setCursor(105, 80);
    display.setTextSize(2);
    display.print("Humidity");
    display.fillRect(105, 105, 70, 30, TFT_BLACK);
    display.setCursor(105, 105);
    display.setTextSize(3);
    display.print(String(humidity) + "%");

    drawLeftArrow();
}

void ManageDisplays::drawLightBulb(int brightness) {
    //Clear Screen
    display.fillScreen(TFT_BLACK);

    int color_map = map(brightness, 0, 4095, 0, 4);
    uint16_t color = TFT_WHITE;

    switch(color_map) {
        case(1):
            color = PALE_YELLOW;
            break;
        case(2):
            color = LIGHT_YELLOW;
            break;
        case(3):
            color = GOLDISH;
            break;
        case(4):
            color = ORANGE;
            break;
    }

    //Display bulb head
    display.fillCircle(50, 50, 30, color);

    //Display neck
    display.fillRect(38, 75, 23, 20, color);

    //Display base
    display.fillRect(38, 90, 23, 15, TFT_DARKGREY);
    display.fillRoundRect(38, 95, 23, 11, 5, TFT_DARKGREY);

    //Inside bulb - "Y"
    display.setTextColor(TFT_DARKGREY, color);
    display.setTextSize(3);
    display.setCursor(43, 55);
    display.print("Y");

    //Text Portion
    display.setTextColor(TFT_WHITE, TFT_BLACK);
    display.setCursor(105, 45);
    display.setTextSize(2);
    display.print("Brightness");
    display.fillRect(105, 70, 70, 30, TFT_BLACK);
    display.setCursor(105, 70);
    display.setTextSize(3);
    display.print(String(brightness));

    drawLeftArrow();
}

//Backward Icon
void ManageDisplays::drawLeftArrow() {
    display.setCursor(5, 120);
    display.setTextSize(2);
    display.print("<-");
}

//Forward Icon
void ManageDisplays::drawRightArrow() {
    display.setCursor(215, 120);
    display.setTextSize(2);
    display.print("->");
}

//Text "Default"
void ManageDisplays::drawDefaultText() {
    display.setCursor(150, 120);
    display.setTextSize(2);
    display.print("Default");
}

//Convert milliseconds into hours, minutes, and seconds
String ManageDisplays::getTime(unsigned long ms) {
    String time;
    unsigned long total = ms / 1000;
    int hours = (total / 3600) % 24;
    int minutes = (total / 60) % 60;
    int seconds = total % 60;
    
    time = String(hours) + ":";
    if(minutes < 10) {
        time += "0";
    }
    time += String(minutes) + ":";
    if(seconds < 10) {
        time += "0";
    }
    time += String(seconds);
    return time;
}

