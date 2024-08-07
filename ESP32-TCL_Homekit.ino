#include "AC.h"

#define BUTTON_PIN 39
#define DEBOUNCE_TIME 50

TCL_REMOTE *remote;

// Timer
unsigned long previousSensorMillis = 0;
const long sensorInterval = 10000;

// Button
bool lastSteadyState = LOW;
bool lastFlickerableState = LOW;
bool currentState;
unsigned long lastDebounceTime = 0;

void setup() {
  // setCpuFrequencyMhz(120);
  Serial.begin(115200);

  // Sensor
  // Wire.begin(26, 32);
  dht.begin();
  // Remote
  ac.begin();
  ac.setModel(TAC09CHSD);

  // HomeSpan
  homeSpan.enableOTA();
  homeSpan.begin(Category::AirConditioners, "TCL(bedroom) Remote","tclBed");
  homeSpan.enableWebLog(20,"pool.ntp.org","UTC-3"); // Timezone: JST
  new SpanAccessory();
  new Service::AccessoryInformation();
  new Characteristic::Identify();
  remote = new TCL_REMOTE();

  // // Button
  pinMode(BUTTON_PIN, INPUT);
}

void loop() {
  ESP.getFreeHeap();
  // Update the sensor value every 10 seconds.
  if (millis() - previousSensorMillis > sensorInterval) {
    previousSensorMillis = millis();
    remote->updateSensor();
  }

  // Button with debounce.
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

  // Keep this at the bottom.
  homeSpan.poll(); 
}