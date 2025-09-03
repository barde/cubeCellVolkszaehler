/*
 * LoRa Data Structure Definition
 * Shared between CubeCell transmitter and LilyGo receiver
 */

#ifndef LORA_DATA_H
#define LORA_DATA_H

#include <stdint.h>

// Pack struct to ensure no padding bytes
#pragma pack(push, 1)

// Main meter data payload - 20 bytes total
struct MeterData {
  float power_watts;              // Current power in watts (can be negative for generation)
  float total_consumption_kwh;    // Total consumption in kWh (OBIS 1.8.0)
  float total_generation_kwh;     // Total generation in kWh (OBIS 2.8.0)
  float battery_voltage;          // Battery voltage in volts
  uint32_t packet_counter;        // Packet counter to detect missed transmissions
};

// Extended data with link quality (for gateway to HA reporting)
struct MeterDataWithLink {
  MeterData data;
  int16_t rssi;                  // Received Signal Strength Indicator in dBm
  float snr;                     // Signal-to-Noise Ratio in dB
  uint32_t timestamp;            // Unix timestamp when received
};

#pragma pack(pop)

// LoRa Configuration Parameters (must match on both devices)
#define LORA_FREQUENCY      433000000  // 433 MHz (matching LilyGo hardware)
#define LORA_BANDWIDTH      0           // 0: 125 kHz (good balance)
#define LORA_SPREADING_FACTOR 7        // SF7 - back to working config
#define LORA_CODING_RATE    1          // 4/5 coding rate
#define LORA_SYNC_WORD      0x12       // Default LoRa sync word (ESPHome limitation)

// TX Power - can be overridden via build flag
#ifdef LORA_TX_POWER_OVERRIDE
  #define LORA_TX_POWER     LORA_TX_POWER_OVERRIDE
#else
  #define LORA_TX_POWER     20         // Default: Max power for international waters
#endif

#define LORA_PREAMBLE_LENGTH 8         // Standard preamble

// Timing Configuration
#define LORA_TX_TIMEOUT     3000       // TX timeout in ms
#define LORA_RX_TIMEOUT     0          // RX timeout (0 = continuous)

#endif // LORA_DATA_H