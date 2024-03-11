#include "HomeSpan.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_LG.h>

#include "SHT3X.h"
#include "EEPROM.h"

// Remote
const uint16_t kIrLed = 19;
IRLgAc ac(kIrLed);
IRsend irsend(kIrLed);
// Senser
// uint32_t ac_code_to_sent;
// uint32_t arrayOfCodeOfTemp[13] = {0x880834F,0x8808440 ,0x8808541 ,0x8808642 ,0x8808743,0x8808844,0x8808945,0x8808A46,0x8808B47 ,0x8808C48 , 0x8808D49 ,0x8808E4A , 0x8808F4B   };
SHT3X sht30;

EEPROMClass ACState("acState");
EEPROMClass ACTemp("acTemp");

struct LG_REMOTE: Service::Thermostat {

  SpanCharacteristic *currentState;
  SpanCharacteristic *targetState;
  SpanCharacteristic *currentTemp;
  SpanCharacteristic *currentHumidity;
  SpanCharacteristic *targetTemp;
  SpanCharacteristic *unit;

  // Extra characteristic for automation.
  SpanCharacteristic *currentTemp2;

  int previousACState = 0;

  LG_REMOTE()
    : Service::Thermostat() {

    // sht30.get();
    float temperature =20;
    float humidity = 20;

    // Read from EEPROM
    int savedState = 0;
    float savedTemp = 0;

    // EEPROM Initialization.
    if (!ACState.begin(0x4)) {
      Serial.println("Failed to initialise ACState");
      delay(1000);
      ESP.restart();
    }
    if (!ACTemp.begin(0x4)) {
      Serial.println("Failed to initialise ACState");
      delay(1000);
      ESP.restart();
    }

    ACState.get(0, savedState);
    ACTemp.get(0, savedTemp);

    if (savedState == -1) {
      savedState = 0;
    }
    if (isnan(savedTemp)) {
      savedTemp = 22;
    }

    new Characteristic::Name("LG Remote");
    currentState = new Characteristic::CurrentHeatingCoolingState(0);
    targetState = new Characteristic::TargetHeatingCoolingState(savedState);
    currentTemp = new Characteristic::CurrentTemperature(temperature);
    currentHumidity = new Characteristic::CurrentRelativeHumidity(humidity);
    targetTemp = (new Characteristic::TargetTemperature(savedTemp))->setRange(18, 30, 1);
    unit = new Characteristic::TemperatureDisplayUnits(0);

    // Extra temperature sensor for automation.
    new Service::TemperatureSensor();
    new Characteristic::Name("Sensor for Automation");
    currentTemp2 = new Characteristic::CurrentTemperature(temperature);
  }

  void printState() {
    Serial.println("LG A/C remote is in the following state:");
    Serial.printf("  %s\n", ac.toString().c_str());
    WEBLOG("AC Stat  %s\n", ac.toString().c_str());
    // Display the encoded IR sequence.
    unsigned char *ir_code = (unsigned char*)ac.getRaw();
    Serial.print("IR Code: 0x");
    // for (uint8_t i = 0; i < sizeof(ir_code)/sizeof(ir_code[(uint8_t)0]); i++)
    //   Serial.printf("%02X", ir_code[i]);
    Serial.println();
  }

  void updateSensor() {
    // sht30.get();
    float temperature = 20;
    float humidity = 20;
    currentTemp->setVal(temperature);
    currentHumidity->setVal(humidity);

    currentTemp2->setVal(temperature);
  }

  void toggleAC() {
    int state = targetState->getNewVal();
    if (state != 0) {
      previousACState = state;
      targetState->setVal(0);
    } else {
      targetState->setVal(previousACState);
    }
    update();
  }

  // Send IR signal based on the target state.
  boolean update() {
    int state = targetState->getNewVal();
    float temp = targetTemp->getNewVal();

    // Save to EEPROM.
    ACState.put(0, state);
    ACState.commit();
    ACTemp.put(0, temp);
    ACTemp.commit();
    
    if (state == 0) {
      ac.off();
      irsend.sendLG(0x88C0051,28);
      
    } 
    if(state !=0 && previousACState == 0){
      // irsend.sendLG(0x8800549 ,28);
      ac.on();
    }
    if(previousACState != state){
    switch (state) {
      case 0:
        break;
      case 1:
        // irsend.sendLG(0x880CF4F ,28);
        ac.setMode(kLgAcHeat);
        currentState->setVal(state);
        break;
      case 2:
        ac.setMode(kLgAcCool);
        currentState->setVal(state);
        // irsend.sendLG(0x8808642,28);

        break;
      default:
        ac.setMode(kLgAcAuto);
        // Setting the current state to auto cause the device to stop responding.
        currentState->setVal(0);
        break;
    }
    }
          previousACState = state;

    if(state != 0){
    ac.setFan(kLgAcFanAuto);
    ac.setSwingV(kLgAcSwingVAuto);
    ac.setSwingH(kLgAcSwingHAuto);
    ac.setTemp(temp);
    ac.send();
    // ac_code_to_sent = arrayOfCodeOfTemp[(int)(temp-18)];
    // irsend.sendLG(ac_code_to_sent,28);
    }
    printState();
    return (true);
  }
};