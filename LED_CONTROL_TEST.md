# LED Control Test Guide

This guide explains how to test the MQTT LED control functionality with the ESP32 BLE Mesh network.

## System Architecture

```
MQTT Broker (broker.emqx.io) 
    ↓ (LED Topic: 22004015/tranvancon/led)
ESP32 Vendor Client (MQTT Gateway)
    ↓ (BLE Mesh LED Control Message)
ESP32 Vendor Server (DHT Server with Built-in LED)
```

## Components

1. **Vendor Client** (`vendor_models/vendor_client/`):
   - Connects to WiFi and MQTT broker
   - Subscribes to LED control topic: `22004015/tranvancon/led`
   - Forwards LED commands to BLE Mesh server
   - Publishes sensor data to: `22004015/tranvancon`

2. **Vendor Server** (`vendor_models/vendor_server/`):
   - DHT sensor simulation (sends temperature/humidity data)
   - Built-in LED control on GPIO2
   - Receives LED control commands via BLE Mesh

## Testing Steps

### 1. Build and Flash ESP32 Devices

**Vendor Client (MQTT Gateway):**
```bash
cd vendor_models/vendor_client
idf.py build
idf.py flash monitor
```

**Vendor Server (DHT Server with LED):**
```bash
cd vendor_models/vendor_server
idf.py build
idf.py flash monitor
```

### 2. Provision BLE Mesh Network

1. Power on both ESP32 devices
2. The vendor client (provisioner) will automatically discover and provision the vendor server
3. Wait for provisioning to complete (check serial output)
4. Vendor server should start sending DHT data every 5 seconds

### 3. Test LED Control

**Option A: Using Node.js Test Script**
```bash
# Install dependencies (if not already done)
npm install mqtt

# Run the LED test script
node test-mqtt-led.js
```

**Option B: Using MQTT Client (mosquitto)**
```bash
# Turn LED ON
mosquitto_pub -h broker.emqx.io -t "22004015/tranvancon/led" -m "1"

# Turn LED OFF
mosquitto_pub -h broker.emqx.io -t "22004015/tranvancon/led" -m "0"
```

**Option C: Using Dashboard**
1. Start the Node.js server: `npm run dev`
2. Open browser: `http://localhost:3000/dashboard`
3. Login with: `tranvanconkg@gmail.com` / `123`
4. Click the "Toggle LED" button

## Expected Serial Output

### Vendor Client (MQTT Gateway)
```
I (12345) DHT_RECEIVER: MQTT_EVENT_CONNECTED
I (12346) DHT_RECEIVER: mqtt subscribe to LED topic successful, msg_id=1
I (15000) DHT_RECEIVER: MQTT_EVENT_DATA
TOPIC=22004015/tranvancon/led
DATA=1
I (15001) DHT_RECEIVER: === LED Control Message Received ===
I (15002) DHT_RECEIVER: Topic: 22004015/tranvancon/led
I (15003) DHT_RECEIVER: Data: 1
I (15004) DHT_RECEIVER: Parsed LED state: ON
I (15005) DHT_RECEIVER: === Sending LED Control to BLE Mesh Server ===
I (15006) DHT_RECEIVER: Target server address: 0x0002
I (15007) DHT_RECEIVER: LED state: ON
I (15008) DHT_RECEIVER: ✅ Successfully sent LED control to server: ON
```

### Vendor Server (DHT Server with LED)
```
I (15100) DHT_NODE: LED control message received!
I (15101) DHT_NODE: === LED Control Received ===
I (15102) DHT_NODE: LED State: ON
I (15103) DHT_NODE: Timestamp: 15100 ms
I (15104) DHT_NODE: From address: 0x0001
I (15105) DHT_NODE: ============================
I (15106) DHT_NODE: Built-in LED ON
```

## Troubleshooting

### LED Not Responding
1. **Check WiFi Connection**: Vendor client must connect to WiFi first
2. **Check MQTT Connection**: Look for "MQTT_EVENT_CONNECTED" in client logs
3. **Check BLE Mesh Provisioning**: Server must be provisioned by client
4. **Check Server Address**: Client should show valid server address (not 0xFFFF)

### No MQTT Messages
1. **Check Internet Connection**: Both ESP32 and test device need internet
2. **Check MQTT Broker**: Try connecting to broker.emqx.io manually
3. **Check Topic Names**: Ensure exact topic match: `22004015/tranvancon/led`

### BLE Mesh Issues
1. **Check Provisioning**: Both devices should show provisioning complete
2. **Check App Key Binding**: Look for successful app key binding in logs
3. **Check Model Registration**: Vendor models should be properly registered

## MQTT Topics

- **Sensor Data**: `22004015/tranvancon`
  - Format: `{"temperature":25.5, "humidity":60.2, "light_level":1234}`
  
- **LED Control**: `22004015/tranvancon/led`
  - Format: `"0"` (OFF) or `"1"` (ON)

## Hardware

- **Built-in LED**: GPIO2 (most ESP32 boards)
- **DHT Sensor**: Simulated (no physical sensor required)
- **LDR Sensor**: Simulated (no physical sensor required)

## Network Configuration

- **WiFi SSID**: `Tran Con`
- **WiFi Password**: `88888888`
- **MQTT Broker**: `mqtt://broker.emqx.io:1883`

Update these in the source code if needed:
- `vendor_models/vendor_client/main/main.c` lines 58-61 