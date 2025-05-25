# Multi-Sensor Simulation with ESP BLE Mesh Vendor Models

This project demonstrates multiple sensor data simulation using ESP BLE Mesh vendor models.

## Overview

- **DHT Sensor (vendor_server)**: Simulates DHT sensor readings (temperature and humidity) and sends them every 5 seconds
- **LDR Sensor (vendor_server_2)**: Simulates LDR sensor readings (light level, voltage, status) and sends them every 3 seconds  
- **Data Receiver (vendor_client)**: Acts as a provisioner and receives data from both sensors, then prints the received data

## Features

- **DHT sensor simulation** with realistic temperature (20-35°C) and humidity (30-80%) values
- **LDR sensor simulation** with light levels (0-4095), voltage (0-3.3V), and status (Dark/Dim/Bright)
- Periodic data transmission (DHT: 5s, LDR: 3s intervals)
- No pub/sub mechanism used - direct vendor model messaging
- Maintains original provisioning roles (client as provisioner, servers as nodes)
- Supports multiple sensors simultaneously
- No UUID matching restrictions - discovers and provisions any sensor device

## Hardware Setup

1. **DHT Sensor Node**: Flash with `vendor_server` firmware (UUID: 0x32, 0x10)
2. **LDR Sensor Node**: Flash with `vendor_server_2` firmware (UUID: 0x33, 0x11)
3. **Data Receiver**: Flash with `vendor_client` firmware

## Building and Flashing

### Prerequisites
```bash
# Set up ESP-IDF environment first
. $HOME/esp/esp-idf/export.sh
```

### For DHT Sensor:
```bash
cd vendor_models/vendor_server
idf.py build
idf.py flash monitor
```

### For LDR Sensor:
```bash
cd vendor_models/vendor_server_2
idf.py build
idf.py flash monitor
```

### For Data Receiver:
```bash
cd vendor_models/vendor_client
idf.py build
idf.py flash monitor
```

### Build All at Once:
```bash
cd vendor_models
# Set up ESP-IDF environment first
. $HOME/esp/esp-idf/export.sh
./build_all.sh
```

## Operation Flow

1. Start the Data Receiver (vendor_client) first - it will act as the provisioner
2. Start the DHT Sensor (vendor_server) - it will be discovered and provisioned automatically
3. Start the LDR Sensor (vendor_server_2) - it will also be discovered and provisioned automatically
4. After successful provisioning and configuration:
   - DHT Sensor starts sending temperature/humidity data every 5 seconds
   - LDR Sensor starts sending light level data every 3 seconds
   - Data Receiver receives and prints both sensor data with timestamps

## Expected Output

### DHT Sensor (vendor_server) Output:
```
I (12345) DHT_NODE: Sending DHT data - Temp: 23.45°C, Humidity: 65.32%, Time: 12345 ms
I (12345) DHT_NODE: Opcode: 0x0002e5, Size: 12 bytes, Target: 0x0001
I (12345) DHT_NODE: Send 0x0002e5
```

### LDR Sensor (vendor_server_2) Output:
```
I (15345) LDR_NODE: Sending LDR data - Light: 2048, Voltage: 1.65V, Status: Dim, Time: 15345 ms
I (15345) LDR_NODE: Opcode: 0x0003e5, Size: 12 bytes, Target: 0x0001
I (15345) LDR_NODE: Send 0x0003e5
```

### Data Receiver (vendor_client) Output:
```
I (12350) DHT_RECEIVER: Receive publish message 0x0002e5, length: 12
I (12350) DHT_RECEIVER: DHT publish message received!
I (12350) DHT_RECEIVER: === DHT Data Received (Publish) ===
I (12350) DHT_RECEIVER: Temperature: 23.45°C
I (12350) DHT_RECEIVER: Humidity: 65.32%
I (12350) DHT_RECEIVER: Timestamp: 12345 ms
I (12350) DHT_RECEIVER: From address: 0x0005
I (12350) DHT_RECEIVER: ==================================

I (15355) DHT_RECEIVER: Receive publish message 0x0003e5, length: 12
I (15355) DHT_RECEIVER: LDR publish message received!
I (15355) DHT_RECEIVER: === LDR Data Received (Publish) ===
I (15355) DHT_RECEIVER: Light Level: 2048
I (15355) DHT_RECEIVER: Voltage: 1.65V
I (15355) DHT_RECEIVER: Status: Dim
I (15355) DHT_RECEIVER: Timestamp: 15345 ms
I (15355) DHT_RECEIVER: From address: 0x0006
I (15355) DHT_RECEIVER: ===================================
```

## Configuration

### DHT Sensor Configuration:
- **DHT_SEND_INTERVAL_MS**: Modify in `vendor_server/main/main.c` to change sending interval (default: 5000ms)
- **Temperature range**: 20-35°C (modify in `simulate_dht_reading()` function)
- **Humidity range**: 30-80% (modify in `simulate_dht_reading()` function)
- **UUID**: {0x32, 0x10}
- **Opcode**: 0x0002e5

### LDR Sensor Configuration:
- **LDR_SEND_INTERVAL_MS**: Modify in `vendor_server_2/main/main.c` to change sending interval (default: 3000ms)
- **Light level range**: 0-4095 (12-bit ADC simulation)
- **Voltage range**: 0-3.3V (calculated from light level)
- **Status thresholds**: Dark (<1000), Dim (1000-3000), Bright (>3000)
- **UUID**: {0x33, 0x11}
- **Opcode**: 0x0003e5

### Common Configuration:
- **TARGET_NODE_ADDR**: Set to 0x0001 (provisioner address) in both sensor files

## Troubleshooting

If you see "received and send success but not show data when receive":

1. **Check the debug logs** - The receiver should show:
   ```
   I (xxxxx) DHT_RECEIVER: Receive publish message 0xc202e5, length: 12
   I (xxxxx) DHT_RECEIVER: DHT publish message received!
   I (xxxxx) DHT_RECEIVER: === DHT Data Received (Publish) ===
   ```

2. **Verify provisioning** - Both nodes should be provisioned and configured:
   - DHT Node: Shows "DHT timer started successfully"
   - LDR Node: Shows "LDR timer started successfully"
   - Receiver: Shows "Provision and config successfully"

3. **Check app key binding** - Receiver should show:
   ```
   I (xxxxx) DHT_RECEIVER: ESP_BLE_MESH_PROVISIONER_BIND_APP_KEY_TO_MODEL_COMP_EVT, err_code 0
   ```

4. **Monitor sending** - Sensor nodes should show:
   ```
   I (xxxxx) DHT_NODE: Sending DHT data - Temp: 23.45°C, Humidity: 65.32%, Time: 12345 ms
   I (xxxxx) DHT_NODE: Send 0x0002e5
   ```

5. **Check UUID discovery** - Receiver should discover both sensors without UUID restrictions

## Technical Details

### Data Structures:
```c
// DHT sensor data (12 bytes)
typedef struct {
    float temperature;    // 4 bytes
    float humidity;       // 4 bytes
    uint32_t timestamp;   // 4 bytes
} dht_data_t;

// LDR sensor data (12 bytes)
typedef struct {
    uint16_t light_level; // 2 bytes (0-4095)
    float voltage;        // 4 bytes (0-3.3V)
    uint8_t light_status; // 1 byte (0=Dark, 1=Dim, 2=Bright)
    uint32_t timestamp;   // 4 bytes
    uint8_t padding;      // 1 byte (alignment)
} ldr_data_t;
```

### Opcodes:
- DHT Data: `ESP_BLE_MESH_MODEL_OP_3(0x02, CID_ESP)` = 0x0002e5
- LDR Data: `ESP_BLE_MESH_MODEL_OP_3(0x03, CID_ESP)` = 0x0003e5

## Notes

- The vendor servers automatically start sending sensor data after successful provisioning
- Data includes timestamp for latency measurement
- LED indicators show provisioning status (green LED turns off when provisioned)
- No button interaction required - data sending is automatic
- The receiver uses both client and server vendor models to handle sensor data reception
- No UUID matching restrictions allow discovery of any sensor device
- Both sensors can be provisioned and operate simultaneously 