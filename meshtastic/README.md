# Meshtastic Alternative Implementation

This directory contains an alternative implementation using the Meshtastic protocol stack instead of raw LoRa P2P.

## Architecture Overview

```
CubeCell → Meshtastic Protocol → LilyGo (Meshtastic Gateway) → MQTT → Home Assistant
```

## Comparison with LoRa P2P

| Aspect | LoRa P2P (Recommended) | Meshtastic |
|--------|------------------------|------------|
| **Packet Size** | 20 bytes | ~100-150 bytes |
| **Battery Life** | 2-3 months | 1-2 months |
| **Setup Complexity** | Medium | Easy |
| **Protocol Overhead** | Minimal | Significant |
| **Encryption** | Optional | Built-in |
| **Mobile Monitoring** | No | Yes (app) |
| **Mesh Capability** | No | Yes |
| **Custom Data** | Full control | Limited to telemetry |

## When to Use Meshtastic

Choose Meshtastic if you:
- Want to add more nodes in the future (mesh network)
- Need encryption without implementing it yourself
- Want mobile app monitoring
- Plan to use standard environmental sensors
- Prefer using established protocols

## When to Use LoRa P2P

Stay with LoRa P2P if you:
- Need maximum battery life (priority)
- Want minimal latency
- Have custom data requirements
- Need frequent updates (< 5 minutes)
- Want full control over the protocol

## Meshtastic Implementation

### Option 1: Native Meshtastic Firmware

Flash both devices with Meshtastic firmware and configure:
- CubeCell: Would need custom Meshtastic port (not officially supported)
- LilyGo: Supported out of the box

**Problem:** CubeCell (ASR6501) is not officially supported by Meshtastic.

### Option 2: Meshtastic-Compatible Protocol

Implement Meshtastic protobuf format in CubeCell but use direct LoRa:

```cpp
// CubeCell sends Meshtastic-formatted telemetry
TelemetryPacket packet;
packet.time = getTime();
packet.battery_level = getBatteryPercent();
// Custom fields for power data
packet.variant.environment_metrics.voltage = meterData.battery_voltage;
packet.variant.environment_metrics.current = meterData.power_watts;
```

### Option 3: Hybrid Approach (Best of Both)

Use LoRa P2P for the power meter but format data as JSON for easier integration:

```cpp
// Send JSON over LoRa (more bytes but easier parsing)
char json[128];
snprintf(json, sizeof(json), 
  "{\"p\":%.1f,\"c\":%.3f,\"g\":%.3f,\"v\":%.2f,\"n\":%lu}",
  power, consumption, generation, voltage, counter);
```

## Recommended Approach

**For your use case:** Stick with the **LoRa P2P implementation** because:

1. **Battery efficiency** is critical for the CubeCell
2. **Custom power meter data** doesn't fit well in Meshtastic's telemetry model
3. **Simple point-to-point** doesn't need mesh complexity
4. **CubeCell isn't officially supported** by Meshtastic

The LoRa P2P solution is optimized specifically for your smart meter use case, while Meshtastic would add unnecessary overhead for a single sensor node.

## If You Still Want Meshtastic

To use Meshtastic with your setup:

1. **Replace CubeCell** with a supported board (TTGO T-Beam, Heltec V3)
2. **Flash Meshtastic** firmware on both devices
3. **Configure telemetry** module for custom sensors
4. **Setup MQTT** gateway on LilyGo
5. **Configure Home Assistant** with Meshtastic integration

This would give you a standard Meshtastic setup but at the cost of:
- Higher power consumption
- Less optimized for your specific use case
- Need to replace CubeCell hardware