/*
 * LoRa Test Mode Transmitter for CubeCell HTCC-AB01
 * 
 * Sends incremental test data (0-10) for range testing
 * Frequency: 433 MHz for international waters
 */

#include "Arduino.h"
#include "LoRaWan_APP.h"
#include "lora_data.h"


#define DEBUG_SERIAL_BAUD 115200
#define SEND_INTERVAL 5000  // Send every 5 seconds for testing

// Test data variables
MeterData meterData = {0};
uint32_t packetCounter = 0;
uint32_t lastSendTime = 0;
float testCounter = 0.0;

// LoRa state
static RadioEvents_t RadioEvents;
bool loraReady = false;

// Radio event callbacks
void OnTxDone(void) {
  Serial.println("TX Complete");
  Radio.Sleep();
  loraReady = true;
}

void OnTxTimeout(void) {
  Serial.println("TX Timeout!");
  Radio.Sleep();
  loraReady = true;
}

void setupLoRa() {
  Serial.println("=== LoRa Test Mode Setup ===");
  Serial.print("Frequency: ");
  Serial.print(LORA_FREQUENCY / 1000000.0);
  Serial.println(" MHz");
  Serial.print("TX Power: ");
  Serial.print(LORA_TX_POWER);
  Serial.println(" dBm");
  Serial.print("Send Interval: ");
  Serial.print(SEND_INTERVAL / 1000);
  Serial.println(" seconds");
  
  // Radio event callbacks
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  
  Radio.Init(&RadioEvents);
  Radio.SetChannel(LORA_FREQUENCY);
  
  // Set private network (0x12) - this should be the default
  Radio.SetPublicNetwork(false);
  
  // Configure for LoRa P2P
  Radio.SetTxConfig(
    MODEM_LORA,                // Modem type
    LORA_TX_POWER,              // TX power
    0,                          // FSK frequency deviation (not used for LoRa)
    LORA_BANDWIDTH,             // Bandwidth
    LORA_SPREADING_FACTOR,     // Spreading Factor
    LORA_CODING_RATE,           // Coding Rate
    LORA_PREAMBLE_LENGTH,       // Preamble length
    false,                      // Fixed length packets
    true,                       // CRC enabled
    0,                          // Frequency hopping (0 = disabled)
    0,                          // Hop period (not used)
    false,                      // IQ inversion
    LORA_TX_TIMEOUT             // TX timeout
  );
  
  loraReady = true;
  Serial.println("LoRa initialized for test mode");
}

void setup() {
  Serial.begin(DEBUG_SERIAL_BAUD);
  delay(1000);
  
  Serial.println("\n=== CubeCell LoRa Test Mode ===");
  Serial.println("Sending incremental test data");
  Serial.println("Pattern: 0.0 -> 10.0 -> repeat");
  
  // RGB LED indication for power on
  pinMode(RGB, OUTPUT);
  digitalWrite(RGB, HIGH);
  delay(500);
  digitalWrite(RGB, LOW);
  
  setupLoRa();
}

void sendTestData() {
  // Flash LED to indicate transmission
  digitalWrite(RGB, HIGH);
  
  // Generate test data (0.0 to 10.0, then wrap)
  meterData.power_watts = testCounter * 100;  // 0-1000W
  meterData.total_consumption_kwh = testCounter;  // 0-10 kWh
  meterData.total_generation_kwh = testCounter / 2;  // 0-5 kWh
  
  // Read battery voltage
  uint16_t batteryVoltage = getBatteryVoltage();
  meterData.battery_voltage = batteryVoltage / 1000.0;
  
  // Increment packet counter
  packetCounter++;
  meterData.packet_counter = packetCounter;
  
  // Print test data
  Serial.println("\n=== Sending Test Data ===");
  Serial.print("Packet #");
  Serial.println(packetCounter);
  Serial.print("Test Counter: ");
  Serial.println(testCounter);
  Serial.print("Power: ");
  Serial.print(meterData.power_watts);
  Serial.println(" W");
  Serial.print("Consumption: ");
  Serial.print(meterData.total_consumption_kwh);
  Serial.println(" kWh");
  Serial.print("Generation: ");
  Serial.print(meterData.total_generation_kwh);
  Serial.println(" kWh");
  Serial.print("Battery: ");
  Serial.print(meterData.battery_voltage);
  Serial.println(" V");
  
  // Send via LoRa
  loraReady = false;
  Radio.Send((uint8_t*)&meterData, sizeof(MeterData));
  
  // Wait for TX to complete (with timeout)
  uint32_t startTime = millis();
  while (!loraReady && (millis() - startTime < 3000)) {
    Radio.IrqProcess();
    delay(1);
  }
  
  if (loraReady) {
    Serial.print("Packet sent successfully (");
    Serial.print(sizeof(MeterData));
    Serial.println(" bytes)");
  } else {
    Serial.println("TX failed or timed out!");
  }
  
  Serial.println("========================");
  
  // Turn off LED
  digitalWrite(RGB, LOW);
  
  // Increment test counter (0 to 10, then wrap)
  testCounter += 0.5;
  if (testCounter > 10.0) {
    testCounter = 0.0;
    Serial.println("\n*** Test counter wrapped to 0 ***\n");
  }
}

void loop() {
  // Check if it's time to send
  if (millis() - lastSendTime >= SEND_INTERVAL) {
    sendTestData();
    lastSendTime = millis();
  }
  
  // Process LoRa interrupts
  Radio.IrqProcess();
  
  // Small delay to prevent watchdog
  delay(10);
}