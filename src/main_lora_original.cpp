/*
 * LoRa Test Transmitter for CubeCell HTCC-AB01
 * Based on original working volkszahler_cubecell.ino
 * 
 * Sends incremental test data via LoRa
 * Frequency: 433 MHz
 */

#include "Arduino.h"
#include "LoRaWan_APP.h"

#define DEBUG_SERIAL_BAUD 115200
#define SEND_INTERVAL 5000  // Send every 5 seconds

// LoRa parameters - 433 MHz for international waters
#define RF_FREQUENCY 433000000  // Hz
#define TX_OUTPUT_POWER 14      // dBm
#define LORA_BANDWIDTH 0        // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz]
#define LORA_SPREADING_FACTOR 7 // SF7
#define LORA_CODINGRATE 1       // [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
#define LORA_PREAMBLE_LENGTH 8  // preamble length
#define LORA_SYMBOL_TIMEOUT 0   // symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false

// Test data
typedef struct {
  float power_watts;
  float total_consumption_kwh;
  float total_generation_kwh;
  float battery_voltage;
  uint32_t packet_counter;
} MeterData;

MeterData meterData = {0};
uint32_t packetCounter = 0;
uint32_t lastSendTime = 0;
float testCounter = 0.0;

static RadioEvents_t RadioEvents;

void OnTxDone(void) {
  Serial.println("TX done!");
  Radio.Sleep();
}

void OnTxTimeout(void) {
  Serial.println("TX Timeout!");
  Radio.Sleep();
}

void setup() {
  Serial.begin(DEBUG_SERIAL_BAUD);
  delay(1000);
  
  boardInitMcu();
  
  Serial.println("\n=== CubeCell LoRa Test (Original) ===");
  Serial.println("Based on working volkszahler code");
  Serial.print("Frequency: ");
  Serial.print(RF_FREQUENCY / 1000000.0);
  Serial.println(" MHz");
  
  // RGB LED test
  pinMode(RGB, OUTPUT);
  digitalWrite(RGB, HIGH);
  delay(500);
  digitalWrite(RGB, LOW);
  
  // Initialize Radio events
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  
  Radio.SetTxConfig(
    MODEM_LORA,
    TX_OUTPUT_POWER,
    0, // fdev
    LORA_BANDWIDTH,
    LORA_SPREADING_FACTOR,
    LORA_CODINGRATE,
    LORA_PREAMBLE_LENGTH,
    LORA_FIX_LENGTH_PAYLOAD_ON,
    true, // CRC on
    0, // frequency hopping off
    0, // hop period
    LORA_IQ_INVERSION_ON,
    3000 // TX timeout
  );
  
  Serial.println("LoRa initialized");
}

void sendTestData() {
  // Flash LED
  digitalWrite(RGB, HIGH);
  
  // Generate test data
  meterData.power_watts = testCounter * 100;  // 0-1000W
  meterData.total_consumption_kwh = testCounter;
  meterData.total_generation_kwh = testCounter / 2;
  
  // Simple battery voltage reading
  uint16_t batteryVoltage = getBatteryVoltage();
  meterData.battery_voltage = batteryVoltage / 1000.0;
  
  packetCounter++;
  meterData.packet_counter = packetCounter;
  
  Serial.print("\n=== Packet #");
  Serial.print(packetCounter);
  Serial.println(" ===");
  Serial.print("Test: ");
  Serial.println(testCounter);
  Serial.print("Power: ");
  Serial.print(meterData.power_watts);
  Serial.println(" W");
  Serial.print("Battery: ");
  Serial.print(meterData.battery_voltage);
  Serial.println(" V");
  
  // Send via LoRa
  Radio.Send((uint8_t*)&meterData, sizeof(MeterData));
  
  // Wait a bit for TX to complete
  delay(100);
  
  // Turn off LED
  digitalWrite(RGB, LOW);
  
  // Increment test counter
  testCounter += 0.5;
  if (testCounter > 10.0) {
    testCounter = 0.0;
    Serial.println("\n*** Counter wrapped ***\n");
  }
}

void loop() {
  if (millis() - lastSendTime >= SEND_INTERVAL) {
    sendTestData();
    lastSendTime = millis();
  }
  
  Radio.IrqProcess();
  delay(10);
}