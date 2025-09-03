# Volkszähler LoRa Gateway Setup

This directory contains the ESPHome configuration for the LilyGo LoRa32 gateway that receives data from the CubeCell and forwards it to Home Assistant.

## Hardware Setup

### LilyGo LoRa32 Pinout (V2.1)
- **LoRa SX1262 Connections:**
  - SCK: GPIO5
  - MISO: GPIO19
  - MOSI: GPIO27
  - CS: GPIO18
  - DIO1: GPIO26
  - RST: GPIO23
  - BUSY: GPIO32

## Installation Steps

### 1. Install ESPHome
```bash
pip install esphome
```

### 2. Configure WiFi Credentials
```bash
cp secrets.yaml.example secrets.yaml
# Edit secrets.yaml with your WiFi credentials
```

### 3. Download Fonts for Display
```bash
cd fonts
./download_fonts.sh
cd ..
```

### 4. Choose Configuration File
- **With Display**: `lilygo_lora_gateway_display.yaml` (recommended)
- **Without Display**: `lilygo_lora_gateway.yaml` (headless)

### 5. Compile and Upload Firmware
```bash
# For version with OLED display
esphome run lilygo_lora_gateway_display.yaml

# For headless version (no display)
esphome run lilygo_lora_gateway.yaml

# Subsequent updates - Over The Air
esphome run lilygo_lora_gateway_display.yaml --device volkszahler-lora-gateway.local
```

### 6. Add to Home Assistant
1. Go to Settings → Devices & Services
2. ESPHome should auto-discover the gateway
3. Click Configure and enter the API encryption key
4. All sensors will be automatically added

## CubeCell Transmitter Setup

### 1. Compile and Upload LoRa Firmware
```bash
cd ..
pio run -e cubecell_lora --target upload
```

### 2. Monitor Serial Output (Optional)
```bash
pio device monitor -e cubecell_lora
```

## OLED Display Pages

The gateway cycles through three display pages (10 second rotation):

### Page 1: Power & Energy
- Current power consumption/generation (large font)
- Direction indicator (↓ for generation)
- Total consumption and generation
- Last update timer

### Page 2: Link Status
- RSSI signal strength with bar graph
- SNR (Signal-to-Noise Ratio)
- Battery voltage and percentage
- Packet statistics

### Page 3: Daily Statistics
- Current date
- Total consumption
- Total solar generation
- Net energy balance

## Home Assistant Entities

The gateway creates the following entities in Home Assistant:

### Sensors
- **sensor.smart_meter_power** - Current power (W)
- **sensor.smart_meter_total_consumption** - Total consumption (kWh)
- **sensor.smart_meter_total_generation** - Total generation (kWh)
- **sensor.smart_meter_battery_voltage** - Battery voltage (V)

### Diagnostic Sensors
- **sensor.lora_link_rssi** - Signal strength (dBm)
- **sensor.lora_link_snr** - Signal-to-noise ratio (dB)
- **sensor.lora_packet_counter** - Total packets received
- **sensor.lora_missed_packets** - Number of missed packets
- **binary_sensor.lora_connected** - Connection status
- **sensor.lora_status** - Text status
- **sensor.last_packet_time** - Timestamp of last packet

## Energy Dashboard Configuration

To add the meter to Home Assistant's Energy Dashboard:

1. Go to Settings → Dashboards → Energy
2. Under "Electricity grid":
   - Add "Smart Meter Total Consumption" as Grid consumption
   - Add "Smart Meter Total Generation" as Return to grid
3. The current power will show in the dashboard automatically

## Troubleshooting

### No Data Received
1. Check serial output from CubeCell: `pio device monitor -e cubecell_lora`
2. Check ESPHome logs: `esphome logs lilygo_lora_gateway.yaml`
3. Verify both devices use same LoRa parameters (frequency, SF, BW, sync word)
4. Check antenna connections

### Poor Signal Quality
- RSSI should be > -120 dBm for reliable communication
- SNR should be > -20 dB
- Try adjusting antenna position or adding external antenna

### Missed Packets
- Normal to have occasional missed packets
- If frequent, check:
  - Power supply stability
  - Distance between devices
  - Interference from other 868MHz devices

## LoRa Parameters

Both devices are configured with:
- **Frequency:** 868 MHz (EU ISM band)
- **Bandwidth:** 125 kHz
- **Spreading Factor:** 7
- **Coding Rate:** 4/5
- **Sync Word:** 0x34 (private network)
- **TX Power:** 14 dBm (CubeCell), 10 dBm (LilyGo)

## Power Consumption

- **CubeCell:** ~15mA active, <10µA deep sleep
- **LilyGo:** ~100mA (WiFi + LoRa RX)
- **Battery Life (CubeCell):** 
  - With 2000mAh battery: ~2-3 months
  - Depends on transmission interval and meter read frequency

## License

MIT