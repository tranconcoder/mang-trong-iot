import mqtt, { MqttClient } from "mqtt";
import SensorData, { IDHTData, ILDRData, ILEDData } from "@/models/sensor-data.model";

class MQTTService {
  private client: MqttClient | null = null;
  private readonly brokerUrl = "mqtt://broker.emqx.io:1883";
  private readonly topics = {
    sensorData: "22004015/tranvancon",
    ledControl: "22004015/tranvancon/led"
  };

  constructor() {
    this.connect();
  }

  private connect(): void {
    console.log("Connecting to MQTT broker...");
    
    this.client = mqtt.connect(this.brokerUrl, {
      clientId: `nodejs_server_${Math.random().toString(16).substr(2, 8)}`,
      clean: true,
      connectTimeout: 4000,
      reconnectPeriod: 1000,
    });

    this.client.on("connect", () => {
      console.log("✅ Connected to MQTT broker");
      this.subscribeToTopics();
    });

    this.client.on("error", (error) => {
      console.error("❌ MQTT connection error:", error);
    });

    this.client.on("disconnect", () => {
      console.log("🔌 Disconnected from MQTT broker");
    });

    this.client.on("message", (topic, message) => {
      this.handleMessage(topic, message);
    });
  }

  private subscribeToTopics(): void {
    if (!this.client) return;

    // Subscribe to sensor data topic
    this.client.subscribe(this.topics.sensorData, (err) => {
      if (err) {
        console.error("Failed to subscribe to sensor data topic:", err);
      } else {
        console.log(`📡 Subscribed to sensor data: ${this.topics.sensorData}`);
      }
    });

    // Subscribe to LED control topic
    this.client.subscribe(this.topics.ledControl, (err) => {
      if (err) {
        console.error("Failed to subscribe to LED control topic:", err);
      } else {
        console.log(`💡 Subscribed to LED control: ${this.topics.ledControl}`);
      }
    });
  }

  private async handleMessage(topic: string, message: Buffer): Promise<void> {
    try {
      const messageStr = message.toString();
      console.log(`📨 Received message on ${topic}:`, messageStr);

      if (topic === this.topics.sensorData) {
        await this.processSensorData(messageStr);
      } else if (topic === this.topics.ledControl) {
        await this.processLEDControl(messageStr);
      }
    } catch (error) {
      console.error("Error processing MQTT message:", error);
    }
  }

  private async processSensorData(messageStr: string): Promise<void> {
    try {
      const data = JSON.parse(messageStr);
      
      // Check if it's DHT data
      if (data.temperature !== undefined && data.humidity !== undefined) {
        await this.saveDHTData(data);
      }
      
      // Check if it's LDR data
      if (data.light_level !== undefined) {
        await this.saveLDRData(data);
      }
    } catch (error) {
      console.error("Error processing sensor data:", error);
    }
  }

  private async saveDHTData(data: any): Promise<void> {
    try {
      const dhtData = new SensorData({
        device_id: "ESP32_DHT_SERVER",
        node_address: "0x0002", // DHT server address
        data_type: "DHT",
        temperature: data.temperature,
        humidity: data.humidity,
        timestamp: data.timestamp || Date.now(),
        raw_data: data
      });

      await dhtData.save();
      console.log("💾 DHT data saved to MongoDB:", {
        temperature: data.temperature,
        humidity: data.humidity,
        timestamp: data.timestamp
      });
    } catch (error) {
      console.error("Error saving DHT data:", error);
    }
  }

  private async saveLDRData(data: any): Promise<void> {
    try {
      const ldrData = new SensorData({
        device_id: "ESP32_LDR_SERVER",
        node_address: "0x0003", // LDR server address
        data_type: "LDR",
        light_level: data.light_level,
        voltage: data.voltage,
        light_status: data.light_status,
        timestamp: data.timestamp || Date.now(),
        raw_data: data
      });

      await ldrData.save();
      console.log("💾 LDR data saved to MongoDB:", {
        light_level: data.light_level,
        voltage: data.voltage,
        light_status: data.light_status,
        timestamp: data.timestamp
      });
    } catch (error) {
      console.error("Error saving LDR data:", error);
    }
  }

  private async processLEDControl(messageStr: string): Promise<void> {
    try {
      const data = JSON.parse(messageStr);
      
      const ledData = new SensorData({
        device_id: "ESP32_LED_CONTROL",
        node_address: "0x0002", // DHT server address (has LED)
        data_type: "LED",
        led_state: data === "1" || data === 1 ? 1 : 0,
        timestamp: Date.now(),
        raw_data: { original_message: messageStr }
      });

      await ledData.save();
      console.log("💾 LED control data saved to MongoDB:", {
        led_state: ledData.led_state,
        timestamp: ledData.timestamp
      });
    } catch (error) {
      console.error("Error processing LED control:", error);
    }
  }

  // Method to publish LED control commands
  public publishLEDControl(state: number): void {
    if (!this.client || !this.client.connected) {
      console.error("MQTT client not connected");
      return;
    }

    const message = state.toString();
    this.client.publish(this.topics.ledControl, message, (err) => {
      if (err) {
        console.error("Failed to publish LED control:", err);
      } else {
        console.log(`💡 Published LED control: ${message}`);
      }
    });
  }

  // Method to get latest sensor data
  public async getLatestSensorData(dataType?: string, limit: number = 10) {
    try {
      const query = dataType ? { data_type: dataType } : {};
      const data = await SensorData.find(query)
        .sort({ received_at: -1 })
        .limit(limit)
        .lean();
      
      return data;
    } catch (error) {
      console.error("Error fetching sensor data:", error);
      return [];
    }
  }

  // Method to get sensor data by time range
  public async getSensorDataByTimeRange(
    startTime: Date, 
    endTime: Date, 
    dataType?: string
  ) {
    try {
      const query: any = {
        received_at: { $gte: startTime, $lte: endTime }
      };
      
      if (dataType) {
        query.data_type = dataType;
      }

      const data = await SensorData.find(query)
        .sort({ received_at: -1 })
        .lean();
      
      return data;
    } catch (error) {
      console.error("Error fetching sensor data by time range:", error);
      return [];
    }
  }

  public disconnect(): void {
    if (this.client) {
      this.client.end();
      console.log("🔌 MQTT client disconnected");
    }
  }
}

export default new MQTTService(); 