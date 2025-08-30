#include "dataSend.h"
#include <Arduino.h>
#include <Button2.h>
#include "display.h"
#include <DHT20.h>
#include <Wire.h>

//Interval
const int TELEMETRY_INTERVAL = 5000; //Get data every 5 seconds
const int NOTIF_INTERVAL = 1000; //Buzzer & LED goes on and off every second
const int REMINDER_INTERVAL = 1000; //If on home page, update screen every second

//Display
ManageDisplays display;

//Buttons
const int LEFT_BUTTON_PIN = 32;
const int RIGHT_BUTTON_PIN = 2;
const int UP_BUTTON_PIN = 17;
const int DOWN_BUTTON_PIN = 15;

Button2 left_button;
Button2 right_button;
Button2 up_button;
Button2 down_button;

//LED Pins
const int RED_PIN = 33; //Do Not Disturb
const int GREEN_PIN = 25; //Stretch Timer
const int BLUE_PIN = 26; //Water Timer
const int YELLOW_PIN = 27; //LED Light Source

//Buzzer
const int BUZZER_PIN = 12;

//Light Sensor
const int MAX_BRIGHTNESS = 4095;
const int MAX_LED_BRIGHTNESS = 255;
const int MIN_BRIGHTNESS = 0;
const int LIGHT_SENSOR_PIN = 36;

//Temperature/Humidity Sensor
DHT20 temp_hum_sensor;

//Enumerated Screen Values 
const int SCREEN_NUM = 6;
const int HOME = 0;
const int MENU = 1;
//Pages from the menu
const int VIEW_TEMPHUM = 2;
const int VIEW_BRI = 3;
const int CHNG_S = 4;
const int CHNG_W = 5;
const int CHNG_B = 6;

//Enumerated Values (Menu Page)
const int MENU_NUM = 5;
const int MENU_V_TEMPHUM = 0;
const int MENU_V_BRI = 1;
const int MENU_CHNG_S = 2;
const int MENU_CHNG_W = 3;
const int MENU_CHNG_B = 4;

//Variables
int curr_screen; //Keep track of screen number
int curr_menu; //Kepp track of selected menu
bool stretch_notif; //True: Pending stretch notification to be resolved
bool water_notif; //True: Pending water notification to be resolved
bool doNotDisturb; //True: No buzzer or LEDs notifications | False: Notifications via sound & light
bool stretch_led_state; //Control whether stretch LED should be on or off
bool water_led_state; //Control whether water LED should be on or off
bool buzzer_state; //Control whether buzzer should be on or off

//Timers
unsigned long home_timer = 0; //Keep track of when to update home screen
unsigned long notif_timer = 0; //Keep track of the when the buzzer and LED should turn on/off
unsigned long lastTelemetryTime = 0;

//Variables to store sensor data read
float temperature = 0;
float humidity = 0;
int brightness = 0;

//Return time converted from minutes to ms
unsigned long toMS(int minutes) {
  return minutes * 60000;
}

//Return the duration and starting time of the timer about to end earlier
Menus closerTimer() {
  unsigned long now = millis();
  unsigned long stretch = display.stretch_menu.timer + toMS(display.stretch_menu.current) - now;
  unsigned long water = display.water_menu.timer + toMS(display.water_menu.current) - now;
  return stretch < water ? display.stretch_menu : display.water_menu;
}

//Get current humidity and temperature
void getTempHumData() {
  if(temp_hum_sensor.read() == DHT20_OK) {
      temperature = temp_hum_sensor.getTemperature();
      humidity = temp_hum_sensor.getHumidity();
  }
}

//Get curent brightness
void getLightData() {
  brightness = analogRead(LIGHT_SENSOR_PIN);
}

//Set default stretch timer
void setDefaultStretchTimer() {
  display.stretch_menu.current = 60;
  display.stretch_menu.timer = millis();
}

//Read current sensor values to set default timers for water breaks
//Calulates a simplified heat index to set default water break timer
void setDefaultWaterTimer() {
  float fahrenheit = (temperature * 9.0 / 5.0) + 32.0;
  float heat_index = 0.5 * (fahrenheit + 61.0 + ((fahrenheit - 68.0) * 1.2) + (humidity * 0.094));

  if(heat_index <= 80) {
    display.water_menu.current = 60; //60 min
  }
  else if(heat_index <= 85) {
    display.water_menu.current = 45; //45 min
  }
  else if(heat_index <= 90) {
    display.water_menu.current = 30; //30 min
  }
  else if(heat_index <= 95) {
    display.water_menu.current = 15; //15 min
  }
  else if(heat_index <= 100) {
    display.water_menu.current = 10; //10 min
  }
  else {
    display.water_menu.current = 5; //5 min
  }
  display.water_menu.timer = millis();
}

//Read current sensor value to set default brightness of LED pin (representing LED lamp)
void setDefaultLight() {
  int inverted = MAX_BRIGHTNESS - brightness;
  int mapped = map(inverted, MIN_BRIGHTNESS, MAX_BRIGHTNESS, MIN_BRIGHTNESS, MAX_LED_BRIGHTNESS);
  //Round the mapped values to match the incrementation provided in the brightness menu
  int rounded = (mapped + display.brightness_menu.increment / 2) / display.brightness_menu.increment * display.brightness_menu.increment;
  display.brightness_menu.current = rounded;
  analogWrite(YELLOW_PIN, rounded);
}

//Restart timer for next reminder
void resetTimer(Menus& item) {
  if(item.type == STRETCH) {
    if(item.isDefault) {
      setDefaultStretchTimer();
    }
    else {
      item.timer = millis();
    }
    if(curr_screen == CHNG_S) {
      display.drawReminderSetting(item);
    }
  }
  else if(item.type == WATER) {
    if(item.isDefault) {
      setDefaultWaterTimer();
    }
    else {
      item.timer = millis();
    }
    if(curr_screen == CHNG_W) {
      display.drawReminderSetting(item);
    }
  }
}

//Event Handlers -- Buttons
void handleUpTap(Button2& b) {
  //If notif on, stop buzzer and led blinking
  if(stretch_notif || water_notif) {
    if(stretch_notif) {
      resetTimer(display.stretch_menu);
      digitalWrite(GREEN_PIN, LOW);
      stretch_notif = false;

      if(curr_screen == CHNG_S) {
        display.drawReminderSetting(display.stretch_menu);
      }
    }
    if(water_notif) {
      resetTimer(display.water_menu);
      digitalWrite(BLUE_PIN, LOW);
      water_notif = false;

      if(curr_screen == CHNG_W) {
        display.drawReminderSetting(display.water_menu);
      }
    }
    noTone(BUZZER_PIN);
  }
  else {
    if(b.wasPressedFor() > 200) {
      //Long Press - Handle "Do Not Disturb"
      doNotDisturb = !doNotDisturb;
      if(doNotDisturb) {
        digitalWrite(RED_PIN, HIGH);
      }
      else {
        //Restart timers when doNotDistrub turned off
        display.stretch_menu.timer = millis();
        display.water_menu.timer = millis();
        digitalWrite(RED_PIN, LOW);
      }

      if(curr_screen == HOME) {
        Menus temp = closerTimer();
        display.drawHome(toMS(temp.current), temp.timer, !doNotDisturb && (display.stretch_menu.isOn || display.water_menu.isOn));      }
    }
    else if(curr_screen == MENU){
      curr_menu = (curr_menu - 1 + MENU_NUM) % MENU_NUM;
      display.drawMenu(curr_menu);
    }
    else if(curr_screen == CHNG_S) {
      //Increment Current Strech Value
      display.stretch_menu.current = (display.stretch_menu.current + display.stretch_menu.increment) % display.stretch_menu.max_val;
      display.stretch_menu.timer = millis();
      display.stretch_menu.isDefault = false;
      if(display.stretch_menu.current == 0) {
        display.stretch_menu.isOn = false;
      }
      display.drawReminderSetting(display.stretch_menu);
    }
    else if(curr_screen == CHNG_W) {
      //Increment Current Water Value
      display.water_menu.current = (display.water_menu.current + display.water_menu.increment) % display.water_menu.max_val;
      display.water_menu.timer = millis();
      display.water_menu.isDefault = false;
      if(display.water_menu.current == 0) {
        display.water_menu.isOn = false;
      }
      display.drawReminderSetting(display.water_menu);
    }
    else if(curr_screen == CHNG_B) {
      //Increment Current Brightness Value
      display.brightness_menu.current = (display.brightness_menu.current + display.brightness_menu.increment) % display.brightness_menu.max_val;
      analogWrite(YELLOW_PIN, display.brightness_menu.current);
      display.brightness_menu.isDefault = false;
      display.drawLightSetting(display.brightness_menu);
    }
  }
}

void handleDownTap(Button2& b) {
  //If notif on, stop buzzer and led blinking
  if(stretch_notif || water_notif) {
    if(stretch_notif) {
      resetTimer(display.stretch_menu);
      digitalWrite(GREEN_PIN, LOW);
      stretch_notif = false;

      if(curr_screen == CHNG_S) {
        display.drawReminderSetting(display.stretch_menu);
      }
    }
    if(water_notif) {
      resetTimer(display.water_menu);
      digitalWrite(BLUE_PIN, LOW);
      water_notif = false;

      if(curr_screen == CHNG_W) {
        display.drawReminderSetting(display.water_menu);
      }
    }
    noTone(BUZZER_PIN);
  }
  else {
    if(b.wasPressedFor() > 200) {
      //Long Press - Return to default value
      if(curr_screen == CHNG_S) {
        //Default Current Strech Timer
        setDefaultStretchTimer();
        display.stretch_menu.isDefault = true;
        display.drawReminderSetting(display.stretch_menu);
      }
      else if(curr_screen == CHNG_W) {
        //Default Current Water Timer
        setDefaultWaterTimer();
        display.water_menu.isDefault = true;
        display.drawReminderSetting(display.water_menu);
      }
      else if(curr_screen == CHNG_B) {
        //Default Brightness
        setDefaultLight();
        display.brightness_menu.isDefault = true;
        display.drawLightSetting(display.brightness_menu);
      }
    }
    else if(curr_screen == MENU){
      curr_menu = (curr_menu + 1) % MENU_NUM;
      display.drawMenu(curr_menu);
    }
    else if(curr_screen == CHNG_S) {
      //Increment Current Strech Value
      display.stretch_menu.current = (display.stretch_menu.current + display.stretch_menu.max_val - display.stretch_menu.increment) % display.stretch_menu.max_val;
      display.stretch_menu.timer = millis();
      display.stretch_menu.isDefault = false;
      if(display.stretch_menu.current == 0) {
        display.stretch_menu.isOn = false;
      }
      display.drawReminderSetting(display.stretch_menu);
    }
    else if(curr_screen == CHNG_W) {
      //Increment Current Water Value
      display.water_menu.current = (display.water_menu.current + display.water_menu.max_val - display.water_menu.increment) % display.water_menu.max_val;
      display.water_menu.timer = millis();
      display.water_menu.isDefault = false;
      if(display.water_menu.current == 0) {
        display.water_menu.isOn = false;
      }
      display.drawReminderSetting(display.water_menu);
    }
    else if(curr_screen == CHNG_B) {
      //Increment Current Brightness Value
      display.brightness_menu.current = (display.brightness_menu.current + display.brightness_menu.max_val - display.brightness_menu.increment) % display.brightness_menu.max_val;
      analogWrite(YELLOW_PIN, display.brightness_menu.current);
      display.brightness_menu.isDefault = false;
      display.drawLightSetting(display.brightness_menu);;
    }
  }
}

void handleLeftTap(Button2& b) {
  //If notif on, stop buzzer and led blinking
  if(stretch_notif || water_notif) {
    if(stretch_notif) {
      resetTimer(display.stretch_menu);
      digitalWrite(GREEN_PIN, LOW);
      stretch_notif = false;

      if(curr_screen == CHNG_S) {
        display.drawReminderSetting(display.stretch_menu);
      }
    }
    if(water_notif) {
      resetTimer(display.water_menu);
      digitalWrite(BLUE_PIN, LOW);
      water_notif = false;

      if(curr_screen == CHNG_W) {
        display.drawReminderSetting(display.water_menu);
      }
    }
    noTone(BUZZER_PIN);
  }
  else {
    Menus temp = closerTimer();
    if(b.wasPressedFor() > 200) {
      //Long Press - Return to Home display
      curr_screen = HOME;
      display.drawHome(toMS(temp.current), temp.timer, !doNotDisturb && (display.stretch_menu.isOn || display.water_menu.isOn));
    }
    else {
      //Move through screens
      switch(curr_screen) {
        case HOME:
          break;
        case MENU:
          curr_screen = HOME;
          break;
        default:
          curr_screen = MENU;
          break;
      }

      switch(curr_screen) {
        case HOME:
          display.drawHome(toMS(temp.current), temp.timer, !doNotDisturb && (display.stretch_menu.isOn || display.water_menu.isOn));
          break;
        case MENU:
          display.drawMenu(curr_menu);
          break;
        case VIEW_TEMPHUM:
          display.drawThermometer(temperature);
          display.drawDroplet(humidity);
          break;
        case VIEW_BRI:
          display.drawLightBulb(brightness);
          break;
        case CHNG_S:
          display.drawReminderSetting(display.stretch_menu);
          break;
        case CHNG_W:
          display.drawReminderSetting(display.water_menu);
          break;
        case CHNG_B:
          display.drawLightSetting(display.brightness_menu);
          break;
      }
    }
  }
}

void handleRightTap(Button2& b) {
  //If notif on, stop buzzer and led blinking
  if(stretch_notif || water_notif) {
    if(stretch_notif) {
      resetTimer(display.stretch_menu);
      digitalWrite(GREEN_PIN, LOW);
      stretch_notif = false;

      if(curr_screen == CHNG_S) {
        display.drawReminderSetting(display.stretch_menu);
      }
    }
    if(water_notif) {
      resetTimer(display.water_menu);
      digitalWrite(BLUE_PIN, LOW);
      water_notif = false;

      if(curr_screen == CHNG_W) {
        display.drawReminderSetting(display.water_menu);
      }
    }
    noTone(BUZZER_PIN);
  }
  else {
    Menus temp = closerTimer();
    //Move through screens
    switch(curr_screen) {
      case HOME:
        curr_screen = MENU;
        break;
      case MENU:
        switch(curr_menu) {
          case MENU_V_TEMPHUM:
            curr_screen = VIEW_TEMPHUM;
            break;
          case MENU_V_BRI:
            curr_screen = VIEW_BRI;
            break;
          case MENU_CHNG_S:
            curr_screen = CHNG_S;
            break;
          case MENU_CHNG_W:
            curr_screen = CHNG_W;
            break;
          case MENU_CHNG_B:
            curr_screen = CHNG_B;
            break;
        }
        break;
      }

    switch(curr_screen) {
      case HOME:
        display.drawHome(toMS(temp.current), temp.timer, !doNotDisturb && (display.stretch_menu.isOn || display.water_menu.isOn));
        break;
      case MENU:
        display.drawMenu(curr_menu);
        break;
      case VIEW_TEMPHUM:
        display.drawThermometer(temperature);
        display.drawDroplet(humidity);
        break;
      case VIEW_BRI:
        display.drawLightBulb(brightness);
        break;
      case CHNG_S:
        display.drawReminderSetting(display.stretch_menu);
        break;
      case CHNG_W:
        display.drawReminderSetting(display.water_menu);
        break;
      case CHNG_B:
        display.drawLightSetting(display.brightness_menu);
        break;
    }
  }
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  delay(1000);

  //Start Temp/Humidity Sensor
  temp_hum_sensor.begin();

  //Start Wifi
  startWiFi();

  //Set up LED pins as outputs
  pinMode(RED_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  //Initialize buzzer
  pinMode(BUZZER_PIN, OUTPUT);

  //Initialize buttons
  left_button.begin(LEFT_BUTTON_PIN);
  right_button.begin(RIGHT_BUTTON_PIN);
  up_button.begin(UP_BUTTON_PIN);

  pinMode(DOWN_BUTTON_PIN, INPUT_PULLUP);
  down_button.begin(DOWN_BUTTON_PIN);

  //Set up handlers for buttons
  left_button.setTapHandler(handleLeftTap);
  right_button.setTapHandler(handleRightTap);
  up_button.setTapHandler(handleUpTap);
  down_button.setTapHandler(handleDownTap);

  //Initialize TTGO Display
  display.start();
  curr_screen = HOME;
  curr_menu = MENU_V_TEMPHUM;

  //Get initial data (to use to set timers)
  getTempHumData();
  getLightData();

  //Set up timers
  setDefaultStretchTimer();
  setDefaultWaterTimer();
  setDefaultLight();

  home_timer = millis();
  notif_timer = millis();

  //Ensure usage of ms
  if(display.stretch_menu.current < display.water_menu.current) {
    display.drawHome(toMS(display.stretch_menu.current), notif_timer, true);
  }
  else {
    display.drawHome(toMS(display.water_menu.current), notif_timer, true);
  }
  
  stretch_notif = false;
  water_notif = false;
  stretch_led_state = false;
  water_led_state = false;
  buzzer_state = false;
  doNotDisturb = false;
}

void loop() {
  //Handle button clicks
  left_button.loop();
  right_button.loop();
  up_button.loop();
  down_button.loop();


  //Get sensor data
  if(millis() - lastTelemetryTime >= TELEMETRY_INTERVAL) {
    getTempHumData();
    getLightData();

    //Use default value for brightness if default is used
    if(display.brightness_menu.isDefault) {
      setDefaultLight();
    }

    sendData(temperature, humidity, brightness);

    //Use data to update screen, if at data screens
    switch(curr_screen) {
      case VIEW_TEMPHUM:
        display.drawThermometer(temperature);
        display.drawDroplet(humidity);
        break;
      case VIEW_BRI:
        display.drawLightBulb(brightness);
        break;
      case CHNG_B:
        display.drawLightSetting(display.brightness_menu);
        break;
    }

    lastTelemetryTime = millis();
  }

  //Check for stretch break
  if((!doNotDisturb && display.stretch_menu.isOn) && millis() - display.stretch_menu.timer >= toMS(display.stretch_menu.current)) {
    stretch_notif = true;
  }

  //Check for water break
  if((!doNotDisturb && display.water_menu.isOn) && millis() - display.water_menu.timer >= toMS(display.water_menu.current)) {
    water_notif = true;
  }

  //If notification present, turn on/off buzzer & LED pins
  if((stretch_notif|| water_notif) && millis() - notif_timer >= NOTIF_INTERVAL) {
    notif_timer = millis();
    
    //Flip states
    stretch_led_state = !stretch_led_state;
    water_led_state = !water_led_state;
    buzzer_state = !buzzer_state;
    
    //Turn on/off LED based on states
    digitalWrite(GREEN_PIN, (stretch_notif && stretch_led_state)? HIGH : LOW);
    digitalWrite(BLUE_PIN, (water_notif && water_led_state)? HIGH : LOW);

    if(buzzer_state) {
      tone(BUZZER_PIN, 2048);
    }
    else {
      noTone(BUZZER_PIN);
    }
  }
  
  //If on home page, update screen every second
  if(curr_screen == HOME && millis() - home_timer >= 1000 && (stretch_notif || water_notif)) {
    display.drawHome(0, 0, true);
    home_timer = millis();
  }
  else if(curr_screen == HOME && millis() - home_timer >= 1000) {
    Menus temp = closerTimer();
    display.drawHome(toMS(temp.current), temp.timer, !doNotDisturb && (display.stretch_menu.isOn || display.water_menu.isOn));
    home_timer = millis();
  }
}

