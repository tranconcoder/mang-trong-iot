import { Schema, model } from "mongoose";

export const SENSOR_DATA_MODEL_NAME = "SensorData";
export const SENSOR_DATA_COLLECTION_NAME = "sensor_data";

// DHT sensor data interface
export interface IDHTData {
  temperature: number;
  humidity: number;
  timestamp: number;
}

// LDR sensor data interface
export interface ILDRData {
  light_level: number;
  voltage: number;
  light_status: number; // 0=Dark, 1=Dim, 2=Bright
  timestamp: number;
}

// LED control data interface
export interface ILEDData {
  led_state: number; // 0=OFF, 1=ON
  timestamp: number;
}

// Main sensor data schema
const sensorDataSchema = new Schema(
  {
    device_id: { type: String, required: true },
    node_address: { type: String, required: true },
    data_type: { 
      type: String, 
      required: true, 
      enum: ['DHT', 'LDR', 'LED'] 
    },
    
    // DHT data (temperature & humidity)
    temperature: { type: Number },
    humidity: { type: Number },
    
    // LDR data (light sensor)
    light_level: { type: Number },
    voltage: { type: Number },
    light_status: { type: Number }, // 0=Dark, 1=Dim, 2=Bright
    
    // LED data
    led_state: { type: Number }, // 0=OFF, 1=ON
    
    // Common fields
    timestamp: { type: Number, required: true },
    received_at: { type: Date, default: Date.now },
    
    // Raw MQTT data for debugging
    raw_data: { type: Schema.Types.Mixed }
  },
  {
    timestamps: true,
    collection: SENSOR_DATA_COLLECTION_NAME,
  }
);

// Create indexes for better query performance
sensorDataSchema.index({ device_id: 1, data_type: 1, timestamp: -1 });
sensorDataSchema.index({ received_at: -1 });
sensorDataSchema.index({ node_address: 1, data_type: 1 });

const SensorData = model(SENSOR_DATA_MODEL_NAME, sensorDataSchema);

export default SensorData; 