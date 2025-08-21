/*
 * Volkszaehler IR Reader with CubeCell HTCC-AB01
 * 
 * Connections:
 * - Volkszaehler TX -> CubeCell Pin 4 (GPIO4)
 * - Volkszaehler RX -> CubeCell Pin 5 (GPIO5)
 * 
 * Modes:
 * - Debug Mode: Send data every 5 seconds
 * - Production Mode: Deep sleep with sending every minute
 */

#include "Arduino.h"
#include <SoftwareSerial.h>

#define VZ_RX_PIN GPIO4
#define VZ_TX_PIN GPIO5
#define SERIAL_BAUD 9600
#define DEBUG_SERIAL_BAUD 115200

#define DEBUG_MODE false

#if DEBUG_MODE
  #define SEND_INTERVAL 5000
  #define SLEEP_TIME 5000
#else
  #define SEND_INTERVAL 60000
  #define SLEEP_TIME 60000
#endif

SoftwareSerial vzSerial(VZ_RX_PIN, VZ_TX_PIN);

uint8_t smlBuffer[512];
uint16_t smlBufferIndex = 0;
bool smlMessageComplete = false;

const uint8_t SML_START[] = {0x1B, 0x1B, 0x1B, 0x1B, 0x01, 0x01, 0x01, 0x01};
const uint8_t SML_END[] = {0x1B, 0x1B, 0x1B, 0x1B, 0x1A};

uint32_t currentPower = 0;
uint32_t totalConsumption = 0;
uint32_t lastSendTime = 0;

static TimerEvent_t sleepTimer;
bool lowpower = false;

void setup() {
  Serial.begin(DEBUG_SERIAL_BAUD);
  vzSerial.begin(SERIAL_BAUD);
  
  boardInitMcu();
  
  if(DEBUG_MODE) {
    Serial.println("Volkszaehler CubeCell - DEBUG MODE");
    Serial.println("Send interval: 5 seconds");
  } else {
    Serial.println("Volkszaehler CubeCell - PRODUCTION MODE");
    Serial.println("Send interval: 1 minute with deep sleep");
  }
  
  Serial.print("TX Pin: ");
  Serial.println(VZ_TX_PIN);
  Serial.print("RX Pin: ");
  Serial.println(VZ_RX_PIN);
  
  TimerInit(&sleepTimer, onSleepTimerEvent);
  
  if(!DEBUG_MODE) {
    TimerSetValue(&sleepTimer, SLEEP_TIME);
    TimerStart(&sleepTimer);
  }
}

void loop() {
  if(lowpower) {
    lowPowerHandler();
    return;
  }
  
  readSMLData();
  
  if(millis() - lastSendTime >= SEND_INTERVAL) {
    sendData();
    lastSendTime = millis();
    
    if(!DEBUG_MODE) {
      Serial.println("Entering deep sleep...");
      delay(100);
      lowpower = true;
    }
  }
  
  if(DEBUG_MODE) {
    delay(100);
  }
}

void onSleepTimerEvent() {
  lowpower = false;
  TimerSetValue(&sleepTimer, SLEEP_TIME);
  TimerStart(&sleepTimer);
}

void readSMLData() {
  while(vzSerial.available()) {
    uint8_t inByte = vzSerial.read();
    
    if(smlBufferIndex < sizeof(smlBuffer)) {
      smlBuffer[smlBufferIndex++] = inByte;
    }
    
    if(checkForSMLEnd()) {
      processSMLMessage();
      smlBufferIndex = 0;
      smlMessageComplete = false;
    }
  }
}

bool checkForSMLStart() {
  if(smlBufferIndex < sizeof(SML_START)) {
    return false;
  }
  
  for(uint8_t i = 0; i < sizeof(SML_START); i++) {
    if(smlBuffer[smlBufferIndex - sizeof(SML_START) + i] != SML_START[i]) {
      return false;
    }
  }
  return true;
}

bool checkForSMLEnd() {
  if(smlBufferIndex < sizeof(SML_END)) {
    return false;
  }
  
  for(uint8_t i = 0; i < sizeof(SML_END); i++) {
    if(smlBuffer[smlBufferIndex - sizeof(SML_END) + i] != SML_END[i]) {
      return false;
    }
  }
  return true;
}

void processSMLMessage() {
  currentPower = extractPower();
  totalConsumption = extractConsumption();
  
  if(DEBUG_MODE) {
    Serial.print("Power: ");
    Serial.print(currentPower);
    Serial.println(" W");
    Serial.print("Total: ");
    Serial.print(totalConsumption);
    Serial.println(" kWh");
  }
}

uint32_t extractPower() {
  const uint8_t POWER_OBIS[] = {0x01, 0x00, 0x10, 0x07, 0x00};
  
  for(uint16_t i = 0; i < smlBufferIndex - 10; i++) {
    bool found = true;
    for(uint8_t j = 0; j < sizeof(POWER_OBIS); j++) {
      if(smlBuffer[i + j] != POWER_OBIS[j]) {
        found = false;
        break;
      }
    }
    if(found) {
      uint32_t value = 0;
      uint8_t valueLength = smlBuffer[i + sizeof(POWER_OBIS) + 1] & 0x0F;
      
      for(uint8_t k = 0; k < valueLength && k < 4; k++) {
        value = (value << 8) | smlBuffer[i + sizeof(POWER_OBIS) + 2 + k];
      }
      
      int8_t scaler = (int8_t)smlBuffer[i + sizeof(POWER_OBIS) + 2 + valueLength];
      
      while(scaler < 0) {
        value /= 10;
        scaler++;
      }
      while(scaler > 0) {
        value *= 10;
        scaler--;
      }
      
      return value;
    }
  }
  return 0;
}

uint32_t extractConsumption() {
  const uint8_t CONSUMPTION_OBIS[] = {0x01, 0x00, 0x01, 0x08, 0x00};
  
  for(uint16_t i = 0; i < smlBufferIndex - 10; i++) {
    bool found = true;
    for(uint8_t j = 0; j < sizeof(CONSUMPTION_OBIS); j++) {
      if(smlBuffer[i + j] != CONSUMPTION_OBIS[j]) {
        found = false;
        break;
      }
    }
    if(found) {
      uint32_t value = 0;
      uint8_t valueLength = smlBuffer[i + sizeof(CONSUMPTION_OBIS) + 1] & 0x0F;
      
      for(uint8_t k = 0; k < valueLength && k < 4; k++) {
        value = (value << 8) | smlBuffer[i + sizeof(CONSUMPTION_OBIS) + 2 + k];
      }
      
      int8_t scaler = (int8_t)smlBuffer[i + sizeof(CONSUMPTION_OBIS) + 2 + valueLength];
      
      if(scaler == -3) {
        value = value;
      } else if(scaler == -2) {
        value = value * 10;
      } else if(scaler == -1) {
        value = value * 100;
      }
      
      return value;
    }
  }
  return 0;
}

void sendData() {
  Serial.println("=== Sending Data ===");
  Serial.print("Current Power: ");
  Serial.print(currentPower);
  Serial.println(" W");
  Serial.print("Total Consumption: ");
  Serial.print(totalConsumption / 1000.0, 3);
  Serial.println(" kWh");
  Serial.print("Battery Voltage: ");
  Serial.print(getBatteryVoltage());
  Serial.println(" mV");
  Serial.println("==================");
}