# 1 "C:\\Users\\dench\\AppData\\Local\\Temp\\tmp5wh7d88m"
#include <Arduino.h>
# 1 "C:/Users/dench/OneDrive/Documents/PlatformIO/Projects/240806-153521-upesy_wroom/src/ESP32-TCL_Homekit.ino"
#include "AC.h"

#define BUTTON_PIN 39
#define DEBOUNCE_TIME 50

TCL_REMOTE *remote;


unsigned long previousSensorMillis = 0;
const long sensorInterval = 10000;


bool lastSteadyState = LOW;
bool lastFlickerableState = LOW;
bool currentState;
unsigned long lastDebounceTime = 0;
void setup();
void loop();
#line 18 "C:/Users/dench/OneDrive/Documents/PlatformIO/Projects/240806-153521-upesy_wroom/src/ESP32-TCL_Homekit.ino"
void setup() {

  Serial.begin(115200);



  dht.begin();

  ac.begin();
  ac.setModel(TAC09CHSD);


  homeSpan.enableOTA();
  homeSpan.begin(Category::AirConditioners, "TCL(bedroom) Remote","tclBed");
  homeSpan.enableWebLog(20,"pool.ntp.org","UTC-3");
  new SpanAccessory();
  new Service::AccessoryInformation();
  new Characteristic::Identify();
  remote = new TCL_REMOTE();


  pinMode(BUTTON_PIN, INPUT);
}

void loop() {
  ESP.getFreeHeap();

  if (millis() - previousSensorMillis > sensorInterval) {
    previousSensorMillis = millis();
    remote->updateSensor();
  }


  currentState = !digitalRead(BUTTON_PIN);
  if (currentState != lastFlickerableState) {
    lastDebounceTime = millis();
    lastFlickerableState = currentState;
  }
  if ((millis() - lastDebounceTime) > DEBOUNCE_TIME && lastDebounceTime != 0) {
    if(lastSteadyState == LOW && currentState == HIGH) {
      remote->toggleAC();
    }
    lastSteadyState = currentState;
  }


  homeSpan.poll();
}