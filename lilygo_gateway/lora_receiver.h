/*
 * ESPHome Custom Component for LoRa Receiver
 * For LilyGo LoRa32 with SX1262
 */

#pragma once

#include "esphome.h"
#include <RadioLib.h>
#include <SPI.h>

// Include the shared data structure
struct MeterData {
  float power_watts;
  float total_consumption_kwh;
  float total_generation_kwh;
  float battery_voltage;
  uint32_t packet_counter;
} __attribute__((packed));

// LoRa Configuration (must match transmitter)
#define LORA_FREQUENCY      868.0      // MHz
#define LORA_BANDWIDTH      125.0       // kHz
#define LORA_SPREADING_FACTOR 7
#define LORA_CODING_RATE    5           // 4/5
#define LORA_SYNC_WORD      0x34
#define LORA_PREAMBLE_LENGTH 8

// LilyGo LoRa32 V2.1 pins (adjust for your board)
#define LORA_SCK    5
#define LORA_MISO   19
#define LORA_MOSI   27
#define LORA_CS     18
#define LORA_DIO1   26
#define LORA_RST    23
#define LORA_BUSY   32

class LoRaReceiver : public Component, public CustomAPIDevice {
 private:
  SPIClass spi;
  SX1262 radio = nullptr;
  
  MeterData lastData;
  uint32_t lastPacketCounter = 0;
  uint32_t missedPackets = 0;
  unsigned long lastPacketTime = 0;
  int16_t lastRSSI = 0;
  float lastSNR = 0;
  
  bool loraInitialized = false;
  
 public:
  LoRaReceiver() : spi(VSPI) {}
  
  void setup() override {
    ESP_LOGD("lora_receiver", "Setting up LoRa receiver...");
    
    // Initialize SPI
    spi.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
    
    // Create radio instance
    radio = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY, spi);
    
    // Initialize SX1262
    ESP_LOGD("lora_receiver", "Initializing SX1262...");
    int state = radio.begin(
      LORA_FREQUENCY,
      LORA_BANDWIDTH,
      LORA_SPREADING_FACTOR,
      LORA_CODING_RATE,
      LORA_SYNC_WORD,
      10,  // Output power (dBm)
      LORA_PREAMBLE_LENGTH
    );
    
    if (state == RADIOLIB_ERR_NONE) {
      ESP_LOGI("lora_receiver", "SX1262 initialized successfully!");
      loraInitialized = true;
      
      // Set to receive mode
      state = radio.startReceive();
      if (state == RADIOLIB_ERR_NONE) {
        ESP_LOGI("lora_receiver", "Started receiving on %.1f MHz", LORA_FREQUENCY);
      } else {
        ESP_LOGE("lora_receiver", "Failed to start receive mode: %d", state);
      }
    } else {
      ESP_LOGE("lora_receiver", "Failed to initialize SX1262: %d", state);
      loraInitialized = false;
    }
  }
  
  void loop() override {
    if (!loraInitialized) return;
    
    // Check if packet was received
    int state = radio.checkReceive();
    
    if (state == RADIOLIB_ERR_NONE) {
      // Packet received successfully
      uint8_t buffer[sizeof(MeterData)];
      size_t len = sizeof(buffer);
      
      state = radio.readData(buffer, len);
      
      if (state == RADIOLIB_ERR_NONE && len == sizeof(MeterData)) {
        // Valid packet received
        memcpy(&lastData, buffer, sizeof(MeterData));
        
        // Get signal quality
        lastRSSI = radio.getRSSI();
        lastSNR = radio.getSNR();
        lastPacketTime = millis();
        
        // Check for missed packets
        if (lastPacketCounter > 0) {
          uint32_t expected = lastPacketCounter + 1;
          if (lastData.packet_counter > expected) {
            missedPackets += (lastData.packet_counter - expected);
            ESP_LOGW("lora_receiver", "Missed %d packets", 
                     lastData.packet_counter - expected);
          }
        }
        lastPacketCounter = lastData.packet_counter;
        
        // Log received data
        ESP_LOGI("lora_receiver", "Packet #%d received: Power=%.1fW, Consumption=%.3fkWh, "
                 "Generation=%.3fkWh, Battery=%.2fV, RSSI=%ddBm, SNR=%.1fdB",
                 lastData.packet_counter,
                 lastData.power_watts,
                 lastData.total_consumption_kwh,
                 lastData.total_generation_kwh,
                 lastData.battery_voltage,
                 lastRSSI,
                 lastSNR);
        
        // Publish to Home Assistant sensors
        publishData();
        
        // Restart receive mode
        radio.startReceive();
      } else {
        ESP_LOGW("lora_receiver", "Invalid packet received (len=%d, expected=%d)", 
                 len, sizeof(MeterData));
      }
    } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
      // No packet received (normal condition)
    } else if (state != RADIOLIB_ERR_NONE) {
      // Some error occurred
      ESP_LOGW("lora_receiver", "Receive error: %d", state);
    }
  }
  
  void publishData() {
    // Find sensor components and publish states
    auto power = id(meter_power);
    if (power) power->publish_state(lastData.power_watts);
    
    auto consumption = id(meter_consumption);
    if (consumption) consumption->publish_state(lastData.total_consumption_kwh);
    
    auto generation = id(meter_generation);
    if (generation) generation->publish_state(lastData.total_generation_kwh);
    
    auto battery = id(meter_battery);
    if (battery) battery->publish_state(lastData.battery_voltage);
    
    auto rssi = id(lora_rssi);
    if (rssi) rssi->publish_state(lastRSSI);
    
    auto snr = id(lora_snr);
    if (snr) snr->publish_state(lastSNR);
    
    auto counter = id(packet_counter);
    if (counter) counter->publish_state(lastData.packet_counter);
    
    auto missed = id(missed_packets);
    if (missed) missed->publish_state(missedPackets);
    
    // Update last packet time
    auto last_time = id(last_packet_time);
    if (last_time && id(homeassistant_time).has_state()) {
      auto time = id(homeassistant_time).now();
      char buffer[32];
      strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", time.as_local());
      last_time->publish_state(std::string(buffer));
    }
  }
  
  unsigned long seconds_since_last_packet() {
    if (lastPacketTime == 0) return 999999;  // Never received
    return (millis() - lastPacketTime) / 1000;
  }
  
  void reset_missed_packets() {
    missedPackets = 0;
    ESP_LOGI("lora_receiver", "Missed packet counter reset");
  }
};