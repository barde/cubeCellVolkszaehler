# Volkszaehler CubeCell LoRa Smart Meter Bridge

This project creates a wireless bridge between a Volkszaehler IR reader and Home Assistant using LoRa communication. A Heltec CubeCell reads the smart meter data and transmits it via LoRa to a LilyGo LoRa32 gateway running ESPHome with a real-time OLED display.

## Architecture

```
Smart Meter → IR Reader → CubeCell → LoRa P2P → LilyGo LoRa32 → WiFi/API → Home Assistant
                          (Battery)               (OLED Display + ESPHome)
```

## Features

- **Smart Meter Reading**: Reads SML protocol data from Volkszaehler IR reader
- **Wireless Transmission**: LoRa P2P communication (868 MHz EU band)
- **Power Efficient**: Deep sleep support for 2-3 months battery life
- **Real-time Display**: OLED display on gateway shows current readings
- **Home Assistant Integration**: Native ESPHome API integration
- **Bidirectional Energy**: Tracks both consumption and generation (solar)
- **Link Quality Monitoring**: RSSI/SNR tracking for both devices
- **Multiple Implementations**: Binary (efficient) or JSON (readable) protocols

## Implementation Options

### 1. LoRa P2P Binary (Recommended) ✅
- **File**: `src/main_lora.cpp`
- **Packet Size**: 20 bytes
- **Battery Life**: 2-3 months
- **Use When**: Maximum battery efficiency is critical

### 2. LoRa P2P JSON
- **File**: `src/main_lora_json.cpp`  
- **Packet Size**: 60-80 bytes
- **Battery Life**: 1-2 months
- **Use When**: Easier debugging and parsing needed

### 3. Original Serial Only
- **File**: `src/main.cpp`
- **Use When**: Testing with USB connection only

### Why Not Meshtastic?
- CubeCell (ASR6501) not officially supported
- 5-7x packet overhead reduces battery life
- Overkill for single sensor point-to-point
- See `meshtastic/README.md` for detailed comparison

## Hardware Requirements

### Transmitter
- **Heltec CubeCell HTCC-AB01** Dev Board
- **Volkszaehler IR** reading head
- **Battery**: Optional 3.7V LiPo (2000mAh recommended)
- **Antenna**: 868 MHz (included with CubeCell)

### Gateway
- **LilyGo LoRa32 V2.1** with SX1262
- **OLED Display**: 0.96" 128x64 (built-in)
- **Power**: USB or battery
- **WiFi**: For Home Assistant connection

## Wiring

### CubeCell Connections
| Volkszaehler | CubeCell |
|-------------|----------|
| TX          | GPIO4    |
| RX          | GPIO5    |
| GND         | GND      |
| VCC         | 3V3      |

### LilyGo LoRa32 (No wiring needed - all integrated)
- LoRa SX1262: Connected via SPI
- OLED Display: Connected via I2C
- WiFi: ESP32 built-in

## Quick Start

### 1. CubeCell Setup
```bash
# Build and upload LoRa firmware
pio run -e cubecell_lora --target upload

# Monitor output (optional)
pio device monitor -e cubecell_lora
```

### 2. LilyGo Gateway Setup
```bash
cd lilygo_gateway

# Configure WiFi
cp secrets.yaml.example secrets.yaml
nano secrets.yaml  # Add your WiFi credentials

# Install and upload ESPHome
pip install esphome
esphome run lilygo_lora_gateway.yaml
```

### 3. Home Assistant
- Gateway auto-discovers in ESPHome integration
- Add sensors to Energy Dashboard
- Configure automations for low battery alerts

## Display Information

The LilyGo gateway OLED shows:
- **Current Power**: Real-time consumption/generation
- **Daily Energy**: Today's totals
- **Battery Status**: CubeCell battery level
- **Link Quality**: RSSI and SNR
- **Last Update**: Time since last packet

## Data Transmitted

| Field | Description | Unit | Range |
|-------|-------------|------|-------|
| Power | Current power draw/generation | W | ±10000 |
| Consumption | Total energy consumed | kWh | 0-999999 |
| Generation | Total energy generated | kWh | 0-999999 |
| Battery | CubeCell battery voltage | V | 2.0-4.2 |
| RSSI | Signal strength | dBm | -120 to 0 |
| SNR | Signal quality | dB | -20 to +10 |
| Counter | Packet sequence number | - | 0-4294967295 |

## Building Options

```bash
# Binary protocol (most efficient)
pio run -e cubecell_lora --target upload

# JSON protocol (easier debugging)  
pio run -e cubecell_lora_json --target upload

# Original serial-only version
pio run -e cubecell --target upload

# Debug mode (30s interval)
pio run -e cubecell_debug --target upload

# Production mode (60s interval + sleep)
pio run -e cubecell_production --target upload
```

## Configuration

Edit `src/lora_data.h` for LoRa parameters:
```cpp
#define LORA_FREQUENCY      868000000  // 868 MHz EU (915 MHz US)
#define LORA_SPREADING_FACTOR 7        // 7=fast, 12=long range
#define LORA_BANDWIDTH      0           // 0=125kHz, 1=250kHz
#define LORA_TX_POWER       14          // TX power in dBm
```

## Power Consumption

- **CubeCell Active**: ~48mA (during reading/transmission)
- **CubeCell Sleep**: ~3.5µA
- **Battery Life**: 2-3 months with 2000mAh battery (60s interval)
- **LilyGo Gateway**: ~100mA (WiFi + LoRa RX + Display)

## Supported Smart Meters

Compatible with SML protocol meters including:
- **Logarex LK13BE** (3-phase with bidirectional)
- **Logarex LK11BE** (1-phase with bidirectional)
- EMH ED300L
- Iskra MT681
- Easymeter Q3D
- Other SML-compatible meters

## OBIS Codes

| OBIS Code | Description | Unit |
|-----------|-------------|------|
| 1-0:1.8.0 | Total consumption | kWh |
| 1-0:2.8.0 | Total generation | kWh |
| 1-0:16.7.0 | Current power | W |

## Troubleshooting

### No LoRa Reception
1. Check antennas are connected
2. Verify same frequency/SF/BW on both devices
3. Monitor CubeCell: `pio device monitor -e cubecell_lora`
4. Check ESPHome logs: `esphome logs lilygo_lora_gateway.yaml`

### Poor Battery Life
- Increase send interval (currently 60s)
- Check for meter read errors causing retries
- Verify deep sleep is working (production mode)

### Display Not Working
- Check I2C pins in ESPHome config
- Verify display type (SSD1306 128x64)
- Try I2C scan to find address

### No Data from Meter
- Check IR head alignment with meter
- Verify wiring connections
- Confirm meter outputs SML protocol
- Check serial baud rate (9600)

## Home Assistant Energy Dashboard

1. Go to **Settings → Dashboards → Energy**
2. Under **Electricity grid**:
   - Add "Smart Meter Total Consumption" as Grid consumption
   - Add "Smart Meter Total Generation" as Return to grid
3. Current power shows automatically

## License

MIT License - Feel free to modify and use in your projects.