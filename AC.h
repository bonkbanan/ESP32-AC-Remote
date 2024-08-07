#include "HomeSpan.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Tcl.h>

// #include <Wire.h>
#include "DHT.h"
int start =0;
#include "EEPROM.h"
#define DHTTYPE DHT22
// Remote
const uint16_t kIrLed = 19;
IRTcl112Ac ac(kIrLed);
IRsend irsend(kIrLed);
// Senser
// uint32_t ac_code_to_sent;
// uint32_t arrayOfCodeOfTemp[13] = {0x880834F,0x8808440 ,0x8808541 ,0x8808642 ,0x8808743,0x8808844,0x8808945,0x8808A46,0x8808B47 ,0x8808C48 , 0x8808D49 ,0x8808E4A , 0x8808F4B   };
// SHT3X sht30;
DHT dht(23,DHTTYPE);

EEPROMClass ACState("acState");
EEPROMClass ACTemp("acTemp");

struct TCL_REMOTE: Service::Thermostat {

  SpanCharacteristic *currentState;
  SpanCharacteristic *targetState;
  SpanCharacteristic *currentTemp;
  SpanCharacteristic *currentHumidity;
  SpanCharacteristic *targetTemp;
  SpanCharacteristic *unit;

  // Extra characteristic for automation.
  SpanCharacteristic *currentTemp2;

  int previousACState = 0;

  TCL_REMOTE()
    : Service::Thermostat() {

    // sht30.get();
    float temperature =dht.readTemperature() - (float)(1.0);
    float humidity = dht.readHumidity();

    // Read from EEPROM
    int savedState = 0;
    float savedTemp = 0;

    // EEPROM Initialization.
    if (!ACState.begin(0x4) || !ACTemp.begin(0x4)) {
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

    new Characteristic::Name("TCL Remote");
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
    Serial.println("TCL A/C remote is in the following state:");
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
    float temperature = dht.readTemperature() - (float)(1.0);
    float humidity = dht.readHumidity();
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
      // irsend.sendLG(0x88C0051,28);
      
    } 
    if(state !=0 && previousACState == 0){
      // irsend.sendLG(0x8800549 ,28);
      ac.on();
    }
    if(previousACState != state){
    switch (state) {
      case 0:
        currentState->setVal(state);
        break;
      case 1:
        // irsend.sendLG(0x880CF4F ,28);
        ac.setMode(kTcl112AcHeat);
        currentState->setVal(state);
        break;
      case 2:
        ac.setMode(kTcl112AcCool);
        currentState->setVal(state);
        // irsend.sendLG(0x8808642,28);

        break;
      default:
        ac.setMode(kTcl112AcAuto);
        // Setting the current state to auto cause the device to stop responding.
        currentState->setVal(0);
        break;
    }
    }
    if(start != 0){
      uint8_t initialCode[] = {0x23, 0xCB, 0x26, 0x02, 0x00, 0x40, 0x40, 0x98, 0x83, 0x00, 0x00, 0x00, 0x00, 0xC0};
      irsend.sendTcl112Ac(initialCode);

      ac.setFan(kTcl112AcFanAuto);
      ac.setSwingVertical(kTcl112AcSwingVOn);
      ac.setSwingHorizontal(true);
      ac.setTemp(temp);

      ac.send();
      printState();

    }else{
      start++;
    }
    // ac_code_to_sent = arrayOfCodeOfTemp[(int)(temp-18)];
    // irsend.sendLG(ac_code_to_sent,28);
    previousACState = state;

    return (true);
  }
};