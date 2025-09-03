/*
 * Volkszaehler IR Reader with CubeCell HTCC-AB01 and LoRa P2P
 * 
 * Connections:
 * - Volkszaehler TX -> CubeCell Pin 4 (GPIO4)
 * - Volkszaehler RX -> CubeCell Pin 5 (GPIO5)
 * - Internal LoRa SX1262 for data transmission
 * 
 * Modes:
 * - Debug Mode: Send data every 30 seconds
 * - Production Mode: Send data every 60 seconds with deep sleep
 */

#include "Arduino.h"
#include "softSerial.h"
#include "LoRaWan_APP.h"
#include "lora_data.h"

#define VZ_RX_PIN GPIO4
#define VZ_TX_PIN GPIO5
#define SERIAL_BAUD 9600
#define DEBUG_SERIAL_BAUD 115200

#define DEBUG_MODE true

#if DEBUG_MODE
  #define SEND_INTERVAL 30000  // 30 seconds for testing
  #define SLEEP_TIME 30000
#else
  #define SEND_INTERVAL 60000  // 1 minute for production
  #define SLEEP_TIME 60000
#endif

softSerial vzSerial(VZ_RX_PIN, VZ_TX_PIN);

// SML Protocol buffers
uint8_t smlBuffer[512];
uint16_t smlBufferIndex = 0;
bool smlMessageComplete = false;

const uint8_t SML_START[] = {0x1B, 0x1B, 0x1B, 0x1B, 0x01, 0x01, 0x01, 0x01};
const uint8_t SML_END[] = {0x1B, 0x1B, 0x1B, 0x1B, 0x1A};

// Meter data
MeterData meterData = {0};
uint32_t packetCounter = 0;
uint32_t lastSendTime = 0;
uint32_t lastReceiveTime = 0;

// LoRa state
static RadioEvents_t RadioEvents;
bool txDone = false;
bool txTimeout = false;

// Power management
static TimerEvent_t sleepTimer;
bool lowpower = false;

// Forward declarations
void onSleepTimerEvent();
void readSMLData();
bool checkForSMLStart();
bool checkForSMLEnd();
void processSMLMessage();
int32_t extractPower();
uint32_t extractConsumption();
uint32_t extractGeneration();
void sendLoRaData();
void OnTxDone();
void OnTxTimeout();
void setupLoRa();

void setup() {
  Serial.begin(DEBUG_SERIAL_BAUD);
  vzSerial.begin(SERIAL_BAUD);
  
  delay(100);
  
  Serial.println("===================================");
  Serial.println("Volkszaehler CubeCell LoRa Bridge");
  Serial.println("===================================");
  
  if(DEBUG_MODE) {
    Serial.println("Mode: DEBUG (30 second interval)");
  } else {
    Serial.println("Mode: PRODUCTION (60 second interval with sleep)");
  }
  
  Serial.print("Volkszaehler TX Pin: GPIO");
  Serial.println(VZ_TX_PIN);
  Serial.print("Volkszaehler RX Pin: GPIO");
  Serial.println(VZ_RX_PIN);
  
  // Initialize MCU
  boardInitMcu();
  
  // Setup LoRa
  setupLoRa();
  
  // Setup sleep timer
  TimerInit(&sleepTimer, onSleepTimerEvent);
  
  if(!DEBUG_MODE) {
    TimerSetValue(&sleepTimer, SLEEP_TIME);
    TimerStart(&sleepTimer);
  }
  
  Serial.println("Setup complete. Waiting for meter data...");
  Serial.println("-----------------------------------");
}

void loop() {
  if(lowpower) {
    lowPowerHandler();
    return;
  }
  
  // Process LoRa events
  Radio.IrqProcess();
  
  // Read meter data
  readSMLData();
  
  // Check if it's time to send
  if(millis() - lastSendTime >= SEND_INTERVAL) {
    sendLoRaData();
    lastSendTime = millis();
    
    if(!DEBUG_MODE) {
      Serial.println("Entering deep sleep...");
      delay(100);
      lowpower = true;
    }
  }
  
  if(DEBUG_MODE) {
    delay(10);
  }
}

void setupLoRa() {
  Serial.println("Initializing LoRa...");
  
  // Radio events
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  Radio.Init(&RadioEvents);
  
  // Set channel
  Radio.SetChannel(LORA_FREQUENCY);
  
  // Set TX config
  Radio.SetTxConfig(
    MODEM_LORA,                // Modem type
    LORA_TX_POWER,             // TX power
    0,                         // FSK frequency deviation (not used for LoRa)
    LORA_BANDWIDTH,            // Bandwidth
    LORA_SPREADING_FACTOR,    // Spreading factor
    LORA_CODING_RATE,          // Coding rate
    LORA_PREAMBLE_LENGTH,      // Preamble length
    false,                     // Fixed length packets
    true,                      // CRC on
    0,                         // Frequency hopping off
    0,                         // Hop period (not used)
    false,                     // IQ inversion off
    LORA_TX_TIMEOUT            // TX timeout
  );
  
  // Set sync word for private network
  Radio.SetPublicNetwork(false);
  
  Serial.print("LoRa Frequency: ");
  Serial.print(LORA_FREQUENCY / 1000000.0);
  Serial.println(" MHz");
  Serial.print("LoRa SF: ");
  Serial.println(LORA_SPREADING_FACTOR);
  Serial.print("LoRa BW: ");
  Serial.println(LORA_BANDWIDTH == 0 ? "125 kHz" : "250 kHz");
  Serial.print("LoRa TX Power: ");
  Serial.print(LORA_TX_POWER);
  Serial.println(" dBm");
  Serial.println("LoRa initialized successfully");
}

void OnTxDone() {
  txDone = true;
  Serial.println("LoRa TX Complete");
  
  // Get TX stats if available
  int16_t rssi = Radio.Rssi(MODEM_LORA);
  if(rssi != 0) {
    Serial.print("TX RSSI: ");
    Serial.print(rssi);
    Serial.println(" dBm");
  }
}

void OnTxTimeout() {
  txTimeout = true;
  Serial.println("LoRa TX Timeout!");
  Radio.Sleep();
}

void sendLoRaData() {
  Serial.println("\n=== Sending LoRa Data ===");
  
  // Update packet counter
  meterData.packet_counter = ++packetCounter;
  
  // Display data being sent
  Serial.print("Packet #");
  Serial.println(meterData.packet_counter);
  Serial.print("Power: ");
  if(meterData.power_watts < 0) {
    Serial.print("Generating ");
    Serial.print(-meterData.power_watts);
  } else {
    Serial.print("Consuming ");
    Serial.print(meterData.power_watts);
  }
  Serial.println(" W");
  Serial.print("Consumption: ");
  Serial.print(meterData.total_consumption_kwh, 3);
  Serial.println(" kWh");
  Serial.print("Generation: ");
  Serial.print(meterData.total_generation_kwh, 3);
  Serial.println(" kWh");
  Serial.print("Battery: ");
  Serial.print(meterData.battery_voltage, 2);
  Serial.println(" V");
  
  // Check if we have recent data
  if(millis() - lastReceiveTime > 120000 && lastReceiveTime > 0) {
    Serial.println("WARNING: No recent meter data (>2 minutes)");
  }
  
  // Reset flags
  txDone = false;
  txTimeout = false;
  
  // Send the data
  Radio.Send((uint8_t*)&meterData, sizeof(MeterData));
  
  // Wait for TX to complete (with timeout)
  uint32_t startTime = millis();
  while(!txDone && !txTimeout && (millis() - startTime < LORA_TX_TIMEOUT + 1000)) {
    Radio.IrqProcess();
    delay(1);
  }
  
  if(txTimeout) {
    Serial.println("ERROR: Failed to send LoRa packet");
  } else if(txDone) {
    Serial.print("Packet sent successfully (");
    Serial.print(sizeof(MeterData));
    Serial.println(" bytes)");
  }
  
  Serial.println("========================\n");
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
      lastReceiveTime = millis();
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
  int32_t power = extractPower();
  uint32_t consumption = extractConsumption();
  uint32_t generation = extractGeneration();
  
  // Update meter data
  meterData.power_watts = (float)power;
  meterData.total_consumption_kwh = consumption / 1000.0f;
  meterData.total_generation_kwh = generation / 1000.0f;
  meterData.battery_voltage = getBatteryVoltage() / 1000.0f; // Convert mV to V
  
  if(DEBUG_MODE) {
    Serial.println("--- Meter Data Received ---");
    Serial.print("Power: ");
    Serial.print(power);
    Serial.println(" W");
    Serial.print("Consumption: ");
    Serial.print(consumption / 1000.0, 3);
    Serial.println(" kWh");
    Serial.print("Generation: ");
    Serial.print(generation / 1000.0, 3);
    Serial.println(" kWh");
    Serial.println("---------------------------");
  }
}

int32_t extractPower() {
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
      int32_t value = 0;
      uint8_t valueLength = smlBuffer[i + sizeof(POWER_OBIS) + 1] & 0x0F;
      
      // Check if value is signed (MSB set)
      bool isNegative = false;
      if(valueLength > 0 && (smlBuffer[i + sizeof(POWER_OBIS) + 2] & 0x80)) {
        isNegative = true;
        value = -1; // Start with -1 for sign extension
      }
      
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

uint32_t extractGeneration() {
  const uint8_t GENERATION_OBIS[] = {0x01, 0x00, 0x02, 0x08, 0x00};
  
  for(uint16_t i = 0; i < smlBufferIndex - 10; i++) {
    bool found = true;
    for(uint8_t j = 0; j < sizeof(GENERATION_OBIS); j++) {
      if(smlBuffer[i + j] != GENERATION_OBIS[j]) {
        found = false;
        break;
      }
    }
    if(found) {
      uint32_t value = 0;
      uint8_t valueLength = smlBuffer[i + sizeof(GENERATION_OBIS) + 1] & 0x0F;
      
      for(uint8_t k = 0; k < valueLength && k < 4; k++) {
        value = (value << 8) | smlBuffer[i + sizeof(GENERATION_OBIS) + 2 + k];
      }
      
      int8_t scaler = (int8_t)smlBuffer[i + sizeof(GENERATION_OBIS) + 2 + valueLength];
      
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