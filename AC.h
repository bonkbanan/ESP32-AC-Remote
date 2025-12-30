#include <HomeSpan.h>
#include <ir_Tcl.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Wire.h>

// Remote
const uint8_t kIrLed = 19;
IRTcl112Ac ac(kIrLed);
IRsend irsend(kIrLed);

// Sensor
Adafruit_BME280 bme;

struct TCL_REMOTE : Service::Thermostat {
  SpanCharacteristic *currentState;
  SpanCharacteristic *targetState;
  SpanCharacteristic *currentTemp;
  SpanCharacteristic *currentHumidity;
  SpanCharacteristic *targetTemp;
  SpanCharacteristic *unit;

  SpanCharacteristic *currentTemp2;
  SpanCharacteristic *currentHumidity2;

  uint8_t previousACState = 0;

  TCL_REMOTE() : Service::Thermostat() {
    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();

    // HomeKit характеристики
    new Characteristic::Name("TCL Remote");
    currentState = new Characteristic::CurrentHeatingCoolingState(0);
    targetState = new Characteristic::TargetHeatingCoolingState(0);
    currentTemp = new Characteristic::CurrentTemperature(temperature);
    currentHumidity = new Characteristic::CurrentRelativeHumidity(humidity);
    targetTemp = (new Characteristic::TargetTemperature(22))->setRange(18, 30, 1);
    unit = new Characteristic::TemperatureDisplayUnits(0); // 0 = Цельсій

    // Додаткові сервіси для сенсора BME280
    new Service::TemperatureSensor();
    new Characteristic::Name("BME280 Sensor - Temp");
    currentTemp2 = new Characteristic::CurrentTemperature(temperature);

    new Service::HumiditySensor();
    new Characteristic::Name("BME280 Sensor - Humidity");
    currentHumidity2 = new Characteristic::CurrentRelativeHumidity(humidity);

  }

  // Лог поточного стану
  void printState() {
    Serial.println("TCL A/C remote is in the following state:");
    Serial.printf("  %s\n", ac.toString().c_str());
    WEBLOG("AC Stat  %s\n", ac.toString().c_str());
  }

  // Оновлення сенсора
  void updateSensor() {
    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Failed to read from BME280 sensor!");
      WEBLOG("BME280 Error: Failed to read sensor data.");
      return;
    }

    currentTemp->setVal(temperature);
    currentHumidity->setVal(humidity);
    currentTemp2->setVal(temperature);
    currentHumidity2->setVal(humidity);
  }


  // Логіка HomeKit → IR
  boolean update() {
    uint8_t state = targetState->getNewVal();
    float temp = targetTemp->getNewVal();

    bool shouldSendIr = false;

    if (previousACState != state) {
      Serial.printf("AC State changed from %d to %d. Sending IR.\n", previousACState, state);
      WEBLOG("AC State changed from %d to %d. Sending IR.", previousACState, state);
      shouldSendIr = true;
    } else if (state != 0 && ac.getTemp() != temp) {
      Serial.printf("AC On and Temperature changed from %.1f to %.1f. Sending IR.\n", ac.getTemp(), temp);
      WEBLOG("AC On and Temperature changed from %.1f to %.1f. Sending IR.", ac.getTemp(), temp);
      shouldSendIr = true;
    }

    if (shouldSendIr) {
      if (state == 0) {
        ac.off();
        ac.setPower(false);
      } else {
        ac.on();
        ac.setPower(true);
        switch (state) {
          case 1: ac.setMode(kTcl112AcHeat); break;
          case 2: ac.setMode(kTcl112AcCool); break;
          case 3: ac.setMode(kTcl112AcAuto); break;
        }
        ac.setTemp(temp);
        ac.setFan(kTcl112AcFanHigh);
        ac.setSwingVertical(kTcl112AcSwingVOn);
        ac.setSwingHorizontal(true);
      }
      ac.send();
      printState();
    }

    currentState->setVal(state);
    previousACState = state;

    return (true);
  }
};
