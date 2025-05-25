# IoT Sensor Dashboard - Node.js Backend

A Node.js application that connects to MQTT broker, receives sensor data from ESP32 devices, stores data in MongoDB, and provides a web dashboard for monitoring and control.

## Features

- 🌡️ **DHT22 Sensor Data**: Temperature and humidity monitoring
- 💡 **LDR Sensor Data**: Light level monitoring  
- 🔌 **LED Control**: Remote LED control via MQTT
- 📊 **Real-time Dashboard**: Live charts and data visualization
- 🗄️ **MongoDB Storage**: Persistent sensor data storage
- 🔐 **Authentication**: JWT-based API protection
- 📡 **MQTT Integration**: Real-time communication with ESP32 devices

## Prerequisites

- Node.js 18+ or Bun runtime
- MongoDB (local or cloud instance)
- MQTT broker access (using EMQX public broker by default)

## Installation

1. **Install dependencies:**
   ```bash
   bun install
   ```

2. **Set up environment variables:**
   Create a `.env.development` file in the nodejs directory:
   ```env
   # Database Configuration
   MONGODB_URI=mongodb://localhost:27017/mang-trong-iot

   # JWT Configuration
   JWT_SECRET=your-super-secret-jwt-key-change-this-in-production
   JWT_EXPIRES_IN=24h

   # MQTT Configuration
   MQTT_BROKER_URL=mqtt://broker.emqx.io:1883
   MQTT_TOPIC_SENSOR_DATA=22004015/tranvancon
   MQTT_TOPIC_LED_CONTROL=22004015/tranvancon/led

   # Server Configuration
   PORT=3000
   HOST=localhost
   ```

3. **Start MongoDB:**
   Make sure MongoDB is running on your system.

## Usage

### Start the Server

```bash
bun run dev
```

The server will start on `http://localhost:3000`

### Default Login Credentials

- **Email**: `tranvanconkg@gmail.com`
- **Password**: `123`

### Test MQTT Connection

Run the MQTT test script to simulate sensor data:

```bash
node test-mqtt.js
```

This will:
- Connect to the MQTT broker
- Publish simulated DHT and LDR sensor data
- Listen for LED control commands

## API Endpoints

### Authentication
- `POST /api/login` - User login

### Sensor Data
- `GET /api/sensors/latest` - Get latest sensor data
- `GET /api/sensors/dht` - Get DHT sensor data
- `GET /api/sensors/ldr` - Get LDR sensor data
- `GET /api/sensors/stats` - Get sensor statistics
- `GET /api/dashboard/data` - Get dashboard data

### Device Control
- `POST /api/led/control` - Control LED state

### Web Pages
- `GET /` - Home page
- `GET /login` - Login page
- `GET /dashboard` - Sensor dashboard

## MQTT Topics

### Sensor Data Topic: `22004015/tranvancon`

**DHT Data Format:**
```json
{
  "temperature": 25.5,
  "humidity": 60.2,
  "timestamp": 1703123456789
}
```

**LDR Data Format:**
```json
{
  "light_level": 2048,
  "voltage": 1.65,
  "light_status": 1,
  "timestamp": 1703123456789
}
```

### LED Control Topic: `22004015/tranvancon/led`

**LED Control Format:**
```
"0" - Turn LED OFF
"1" - Turn LED ON
```

## Database Schema

### SensorData Collection

```javascript
{
  device_id: String,        // Device identifier
  node_address: String,     // BLE Mesh node address
  data_type: String,        // "DHT", "LDR", or "LED"
  
  // DHT fields
  temperature: Number,
  humidity: Number,
  
  // LDR fields
  light_level: Number,
  voltage: Number,
  light_status: Number,
  
  // LED fields
  led_state: Number,
  
  // Common fields
  timestamp: Number,
  received_at: Date,
  raw_data: Mixed
}
```

## Dashboard Features

1. **Real-time Data**: Live sensor readings via MQTT
2. **Historical Charts**: Temperature, humidity, and light level trends
3. **LED Control**: Toggle LED state remotely
4. **Device Status**: Connection and last update information
5. **Statistics**: Sensor data analytics

## ESP32 Integration

This backend is designed to work with ESP32 BLE Mesh devices:

- **DHT Server** (0x0002): Sends temperature/humidity data
- **LDR Server** (0x0003): Sends light sensor data  
- **Client** (0x0001): MQTT gateway that forwards data to this backend

## Development

### Project Structure

```
nodejs/
├── src/
│   ├── controllers/     # API controllers
│   ├── models/         # MongoDB models
│   ├── services/       # Business logic (MQTT, database)
│   ├── routes/         # Express routes
│   ├── middlewares/    # Custom middlewares
│   ├── configs/        # Configuration files
│   └── views/          # Handlebars templates
├── public/             # Static files (CSS, JS, images)
└── test-mqtt.js        # MQTT testing script
```

### Key Components

- **MQTT Service**: Handles MQTT connections and message processing
- **Sensor Controller**: API endpoints for sensor data
- **Auth Middleware**: JWT authentication
- **Database Service**: MongoDB connection management

## Troubleshooting

### MQTT Connection Issues
- Verify broker URL and port
- Check network connectivity
- Ensure topics match between devices and backend

### Database Issues
- Verify MongoDB is running
- Check connection string in environment variables
- Ensure database permissions

### Authentication Issues
- Verify JWT secret is set
- Check token expiration settings
- Ensure proper headers in API requests

## License

This project is part of an IoT sensor monitoring system for educational purposes.
