# Important Instructions for Claude Code

## Serial Monitoring
⚠️ **WARNING**: Do NOT use Python scripts with serial.Serial() for monitoring serial ports - this causes Claude Code to hang!

Instead, use:
- `pio device monitor` (though it may have terminal issues)
- Web interfaces for monitoring device status
- ESPHome logs via web interface
- curl commands to check device APIs
- File-based logging

## Project Specific Notes
- This project uses 433 MHz LoRa communication (international waters)
- CubeCell HTCC-AB01 v2 as transmitter
- LilyGo LoRa32 SX1262 (433/470 MHz version) as receiver
- ESPHome with external components for custom LoRa implementation