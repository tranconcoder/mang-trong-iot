# DHT Sensor Simulation with ESP BLE Mesh Vendor Models

This project demonstrates DHT sensor data simulation using ESP BLE Mesh vendor models.

## Overview

- **Node 1 (vendor_server)**: Simulates DHT sensor readings (temperature and humidity) and sends them periodically to Node 2
- **Node 2 (vendor_client)**: Acts as a provisioner and receives DHT data from Node 1, then prints the received data

## Features

- DHT sensor simulation with realistic temperature (20-35°C) and humidity (30-80%) values
- Periodic data transmission every 5 seconds
- No pub/sub mechanism used - direct vendor model messaging
- Maintains original provisioning roles (client as provisioner, server as node)

## Hardware Setup

1. **Node 1 (DHT Sensor Node)**: Flash with `vendor_server` firmware
2. **Node 2 (Data Receiver)**: Flash with `vendor_client` firmware

## Building and Flashing

### For Node 1 (DHT Sensor):
```bash
cd vendor_models/vendor_server
idf.py build
idf.py flash monitor
```

### For Node 2 (Data Receiver):
```bash
cd vendor_models/vendor_client
idf.py build
idf.py flash monitor
```

## Operation Flow

1. Start Node 2 (vendor_client) first - it will act as the provisioner
2. Start Node 1 (vendor_server) - it will be discovered and provisioned automatically
3. After successful provisioning and configuration:
   - Node 1 starts sending simulated DHT data every 5 seconds
   - Node 2 receives and prints the DHT data with timestamps

## Expected Output

### Node 1 (DHT Sensor) Output:
```
I (12345) DHT_NODE: Sending DHT data - Temp: 23.45°C, Humidity: 65.32%, Time: 12345 ms
I (17345) DHT_NODE: Sending DHT data - Temp: 24.12°C, Humidity: 67.89%, Time: 17345 ms
```

### Node 2 (Data Receiver) Output:
```
I (12350) DHT_RECEIVER: === DHT Data Received ===
I (12350) DHT_RECEIVER: Temperature: 23.45°C
I (12350) DHT_RECEIVER: Humidity: 65.32%
I (12350) DHT_RECEIVER: Timestamp: 12345 ms
I (12350) DHT_RECEIVER: From address: 0x0005
I (12350) DHT_RECEIVER: ========================
```

## Configuration

- **DHT_SEND_INTERVAL_MS**: Modify in `vendor_server/main/main.c` to change sending interval
- **TARGET_NODE_ADDR**: Set to 0x0001 (provisioner address) in `vendor_server/main/main.c`
- **Temperature/Humidity ranges**: Modify in `simulate_dht_reading()` function

## Troubleshooting

If you see "received and send success but not show data when receive":

1. **Check the debug logs** - The receiver should show:
   ```
   I (xxxxx) DHT_RECEIVER: Receive publish message 0xc202e5, length: 12
   I (xxxxx) DHT_RECEIVER: DHT publish message received!
   I (xxxxx) DHT_RECEIVER: === DHT Data Received (Publish) ===
   ```

2. **Verify provisioning** - Both nodes should be provisioned and configured:
   - Node 1: Shows "DHT timer started successfully"
   - Node 2: Shows "Provision and config successfully"

3. **Check app key binding** - Node 2 should show:
   ```
   I (xxxxx) DHT_RECEIVER: ESP_BLE_MESH_PROVISIONER_BIND_APP_KEY_TO_MODEL_COMP_EVT, err_code 0
   ```

4. **Monitor sending** - Node 1 should show:
   ```
   I (xxxxx) DHT_NODE: Sending DHT data - Temp: 23.45°C, Humidity: 65.32%, Time: 12345 ms
   I (xxxxx) DHT_NODE: Opcode: 0x0002e5, Size: 12 bytes, Target: 0x0001
   I (xxxxx) DHT_NODE: Send 0x0002e5
   ```

## Notes

- The vendor server automatically starts sending DHT data after successful provisioning
- Data includes timestamp for latency measurement
- LED indicators show provisioning status (green LED turns off when provisioned)
- No button interaction required - data sending is automatic
- The receiver uses both client and server vendor models to handle DHT data reception 