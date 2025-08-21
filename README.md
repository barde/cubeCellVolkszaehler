# Volkszaehler CubeCell IR Reader

Simple and power-efficient smart meter reader using Heltec CubeCell HTCC-AB01 and Volkszaehler IR reading head.

## Features

- Ultra-low power consumption (3.5µA in deep sleep)
- SML protocol support for smart meters
- Two operation modes:
  - **Debug Mode**: Sends data every 5 seconds for testing
  - **Production Mode**: Deep sleep with data transmission every minute
- Battery voltage monitoring
- Simple configuration via code

## Hardware Requirements

- Heltec CubeCell HTCC-AB01 Dev Board
- Volkszaehler IR reading head
- USB cable for programming
- Optional: Battery for standalone operation

## Wiring

| Volkszaehler | CubeCell |
|-------------|----------|
| TX          | GPIO4 (Pin 4) |
| RX          | GPIO5 (Pin 5) |
| GND         | GND |
| VCC         | 3V3 |

## Software Setup

### Arduino IDE

1. Add CubeCell board support:
   - Open Arduino IDE Preferences
   - Add to Additional Board Manager URLs:
     ```
     https://github.com/HelTecAutomation/CubeCell-Arduino/releases/download/V1.5.0/package_CubeCell_index.json
     ```
   - Open Board Manager, search for "CubeCell" and install

2. Select Board:
   - Board: "CubeCell-Board (HTCC-AB01)"
   - Region: Select your LoRaWAN region
   - Port: Select the correct COM port

3. Upload the sketch

### PlatformIO (Alternative)

Use the provided `platformio.ini` file for easier development.

## Configuration

Edit the following in `volkszahler_cubecell.ino`:

```cpp
#define DEBUG_MODE false  // Set to true for debug mode
```

### Debug Mode
- Sends data every 5 seconds
- No deep sleep
- Detailed serial output
- Good for testing and development

### Production Mode
- Sends data every minute
- Deep sleep between readings
- Minimal serial output
- Optimized for battery operation

## Power Consumption

- Active: ~48mA (during reading/transmission)
- Deep Sleep: ~3.5µA
- Expected battery life with 2000mAh battery: >1 year

## Serial Output

Connect via serial monitor at 115200 baud to see:
- Current power consumption (W)
- Total energy consumption (kWh)
- Battery voltage (mV)
- Debug information (in debug mode)

## Supported Smart Meters

Compatible with SML protocol meters including:
- EMH ED300L
- Iskra MT681
- Easymeter Q3D
- And other SML-compatible meters

## Troubleshooting

1. **No data received**
   - Check wiring connections
   - Verify IR head alignment with meter
   - Confirm meter outputs SML protocol

2. **High power consumption**
   - Ensure production mode is enabled
   - Check for proper deep sleep execution
   - Verify no pin conflicts

3. **Incorrect values**
   - Adjust OBIS codes for your specific meter
   - Check SML parser logic
   - Verify serial baud rate (usually 9600)

## License

MIT License - Feel free to modify and use in your projects.