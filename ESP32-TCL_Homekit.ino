#include "AC.h"

TCL_REMOTE *remote;

// Timer
unsigned long previousSensorMillis = 0;
const long sensorInterval = 3000; // Update sensor every 3 seconds

// Button
bool lastSteadyState = LOW;
bool lastFlickerableState = LOW;
bool currentState;
unsigned long lastDebounceTime = 0;

void setup() {
  Serial.begin(115200);

  delay(1000);   // Дати стабілізуватись живленню при запуску з Vin

  // Initialize BME280 and check for sensor
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring, address!");
    while (1) delay(10); // Halt if sensor is not found
  }
  Serial.println("BME280 sensor found and initialized.");

  // Remote
  ac.begin();
  ac.setModel(TAC09CHSD);


  homeSpan.begin(Category::AirConditioners, "TCL(bedroom)", "tclBed");

  // HomeSpan
  homeSpan.enableOTA();
  homeSpan.enableWebLog(20, "pool.ntp.org", "UTC-2", "tcl");

  new SpanAccessory();
  new Service::AccessoryInformation();
  new Characteristic::Identify();
  remote = new TCL_REMOTE();
}

void loop() {
  homeSpan.poll();

  // Update the sensor value every 3 seconds.
  if (millis() - previousSensorMillis > sensorInterval) {
    previousSensorMillis = millis();
    remote->updateSensor();
  }

  // Log status every 30 minutes
  static unsigned long lastLogTime = 0;
  unsigned long currentMillis = millis();

  if (currentMillis - lastLogTime >= 1800000) {
    lastLogTime = currentMillis;

    long rssi = WiFi.RSSI();

    WEBLOG("ESP Status: Free Heap = %d bytes", ESP.getFreeHeap());
    WEBLOG("Signal Strength: %d dBm", rssi);
    WEBLOG("Temperature: %.2f °C", remote->currentTemp->getVal());
    WEBLOG("Humidity: %.2f %%", remote->currentHumidity->getVal());
  }

}
