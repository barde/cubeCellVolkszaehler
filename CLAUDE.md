# Important Instructions for Claude Code

## Project Overview
This is a LoRa bridge project connecting a Volkszähler smart meter IR reader to Home Assistant:
- **CubeCell HTCC-AB01 v2**: Battery-powered transmitter reading SML data from smart meter
- **LilyGo LoRa32 T3 v1.6.1**: ESP32 gateway with SX1262 LoRa chip, OLED display, running ESPHome
- **Communication**: LoRa P2P at 433 MHz (international waters frequency)
- **Purpose**: Long-range, battery-efficient smart meter data transmission

## Critical Warnings

### ⚠️ Serial Port Monitoring
**NEVER** use Python scripts with `serial.Serial()` - this causes Claude Code to hang!

Safe alternatives:
```bash
# For CubeCell monitoring
pio device monitor --port /dev/cu.usbserial-5A5B0099371 --baud 115200

# For ESPHome/LoRa32 logs
cd lilygo_gateway
esphome logs lilygo_lora_receiver.yaml --device /dev/cu.usbserial-0001

# For network-based monitoring
esphome logs lilygo_lora_receiver.yaml --device volkszahler-lora-gateway.local
```

### ⚠️ Known Hardware Issues
1. **CubeCell v2 Battery Voltage Bug**: The HTCC-AB01 v2 has a hardware defect where battery voltage reads incorrectly (shows ~1.2V instead of 3.7-4.2V). This CANNOT be fixed in software.
2. **Bootloader Timeout**: CubeCell often shows "Timed out waiting for Bootloader response" but upload succeeds anyway - check for "SUCCESS" message.
3. **LoRa32 Upload Mode**: Must hold BOOT button while pressing RST to enter upload mode if serial upload fails.

## Project Structure

```
volkszahlerCubeCell/
├── src/                          # CubeCell source files
│   ├── main.cpp                 # Original Volkszähler reader (USB only)
│   ├── main_lora.cpp            # LoRa P2P transmitter (production)
│   ├── main_lora_testmode.cpp  # Test transmitter (incremental data)
│   ├── main_lora_original.cpp  # Simplified working version
│   ├── main_simple_test.cpp    # LED blink test
│   └── lora_data.h              # LoRa configuration and data structures
├── lilygo_gateway/              # ESPHome LoRa32 receiver
│   ├── components/lora_receiver/  # Custom ESPHome component
│   ├── lilygo_lora_receiver.yaml  # Main ESPHome config
│   └── secrets.yaml             # WiFi credentials (not in git)
├── platformio.ini               # CubeCell build configurations
└── CLAUDE.md                    # This file
```

## Build Environments

### CubeCell (PlatformIO)
```bash
# Test mode - sends incremental test data every 5 seconds
pio run -e cubecell_testmode --target upload

# Original working version based on volkszahler code
pio run -e cubecell_original --target upload

# Production LoRa mode
pio run -e cubecell_lora --target upload

# Simple LED blink test
pio run -e cubecell_simple --target upload
```

### LoRa32 Gateway (ESPHome)
```bash
cd lilygo_gateway

# Compile and upload via USB
esphome run lilygo_lora_receiver.yaml --device /dev/cu.usbserial-0001

# Upload via OTA (if device is online)
esphome run lilygo_lora_receiver.yaml --device volkszahler-lora-gateway.local

# View logs
esphome logs lilygo_lora_receiver.yaml
```

## LoRa Configuration
Both devices MUST use identical settings (defined in `src/lora_data.h`):
- **Frequency**: 433 MHz (433000000 Hz) for international waters
- **Spreading Factor**: 7 (SF7)
- **Bandwidth**: 125 kHz (0)
- **Coding Rate**: 4/5 (1)
- **Sync Word**: 0x12 (private network)
- **TX Power**: 14 dBm
- **Preamble**: 8 symbols

## Data Structure
```cpp
typedef struct {
  float power_watts;              // Current power (-10000 to 10000)
  float total_consumption_kwh;    // Total consumed energy
  float total_generation_kwh;     // Total generated energy (solar)
  float battery_voltage;          // CubeCell battery (broken on v2)
  uint32_t packet_counter;        // Packet sequence number
} MeterData;  // 20 bytes total
```

## Debugging Guide

### CubeCell Not Transmitting
1. Check LED flashes every 5 seconds (test mode)
2. Verify with: `pio device monitor --port /dev/cu.usbserial-5A5B0099371`
3. Press RST button after upload
4. Try manual reset: Hold USER, press RST, release USER

### LoRa32 Not Receiving
1. Check if hanging: No web interface, no serial output
2. Press RST button to restart
3. Check logs for heartbeat messages (every 10s)
4. Verify pins: DIO1=26, RST=23, BUSY=33

### ESPHome Upload Fails
```bash
# Manual upload mode:
# 1. Hold BOOT button
# 2. Press and release RST
# 3. Release BOOT
# 4. Run upload command
```

### Version Reverting
If new code breaks functionality:
```bash
# Use the original working version
pio run -e cubecell_original --target upload
```

## Important Code Patterns

### Do NOT use blocking loops
```cpp
// BAD - can cause infinite loop
while (digitalRead(busy_pin_)) {
  delay(1);
}

// GOOD - with timeout
uint32_t start = millis();
while (digitalRead(busy_pin_)) {
  if (millis() - start > 3000) {
    ESP_LOGE(TAG, "Timeout!");
    break;
  }
  delay(1);
}
```

### Always add heartbeat logging
```cpp
// In loop() functions
static uint32_t last_heartbeat = 0;
if (millis() - last_heartbeat > 10000) {
  ESP_LOGI(TAG, "Heartbeat: uptime=%lu s", millis() / 1000);
  last_heartbeat = millis();
}
```

## Testing Sequence

1. **Test CubeCell transmission**:
   ```bash
   pio run -e cubecell_testmode --target upload
   pio device monitor  # Should see packets every 5s
   ```

2. **Test LoRa32 reception**:
   ```bash
   cd lilygo_gateway
   esphome logs lilygo_lora_receiver.yaml
   # Should see heartbeat every 10s and packet reception
   ```

3. **Check web interface**:
   ```bash
   curl http://192.168.178.37/  # Or device IP
   ```

## Common Issues & Solutions

| Issue | Solution |
|-------|----------|
| CubeCell battery shows 1.2V | Hardware bug on v2, cannot fix |
| "Bootloader timeout" during upload | Normal, check for SUCCESS message |
| LoRa32 hangs | Press RST button, check debug logs |
| No LoRa reception | Verify both devices use 433 MHz |
| ESPHome won't upload | Use manual BOOT+RST procedure |

## Git Workflow
```bash
# Check status
git status

# Stage all changes
git add -A

# Commit with detailed message
git commit -m "Description of changes"

# Push to GitHub
git push origin master
```

## Home Assistant Integration
The LoRa32 gateway auto-discovers in ESPHome. Sensors available:
- `sensor.smart_meter_power` - Current power (W)
- `sensor.smart_meter_total_consumption` - Total consumed (kWh)
- `sensor.smart_meter_total_generation` - Total generated (kWh)
- `sensor.smart_meter_battery_voltage` - Battery level (V)
- `sensor.lora_rssi` - Signal strength (dBm)
- `sensor.lora_snr` - Signal quality (dB)
- `sensor.lora_packet_counter` - Packets received
- `sensor.lora_missed_packets` - Lost packets

## Performance Notes
- CubeCell draws ~48mA active, ~3.5µA sleeping
- Battery life: 2-3 months with 2000mAh LiPo
- LoRa range: ~2km in urban, ~10km line-of-sight
- Packet size: 20 bytes (binary mode)
- Transmission interval: 5s (test), 60s (production)

## Future Improvements
- [ ] Implement proper SX1262 initialization in LoRa32 receiver
- [ ] Add ACK/retry mechanism for reliability
- [ ] Implement AES encryption for security
- [ ] Add solar panel support for indefinite operation
- [ ] Create PCB design for permanent installation

## Contact & Repository
- GitHub: https://github.com/barde/cubeCellVolkszaehler
- Hardware: CubeCell HTCC-AB01 v2 + LilyGo T3 v1.6.1 SX1262
- Frequency: 433 MHz (international waters, no license required)