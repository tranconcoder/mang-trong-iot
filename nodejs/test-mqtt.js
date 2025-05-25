// Simple MQTT test script
const mqtt = require('mqtt');

const brokerUrl = 'mqtt://broker.emqx.io:1883';
const sensorTopic = '22004015/tranvancon';
const ledTopic = '22004015/tranvancon/led';

console.log('🚀 Starting MQTT test...');

const client = mqtt.connect(brokerUrl, {
  clientId: `test_client_${Math.random().toString(16).substr(2, 8)}`,
  clean: true,
  connectTimeout: 4000,
  reconnectPeriod: 1000,
});

client.on('connect', () => {
  console.log('✅ Connected to MQTT broker');
  
  // Subscribe to LED control topic
  client.subscribe(ledTopic, (err) => {
    if (err) {
      console.error('❌ Failed to subscribe to LED topic:', err);
    } else {
      console.log(`📡 Subscribed to LED control: ${ledTopic}`);
    }
  });

  // Simulate DHT sensor data every 5 seconds
  setInterval(() => {
    const dhtData = {
      temperature: 20 + Math.random() * 15, // 20-35°C
      humidity: 30 + Math.random() * 50,    // 30-80%
      timestamp: Date.now()
    };
    
    client.publish(sensorTopic, JSON.stringify(dhtData), (err) => {
      if (err) {
        console.error('❌ Failed to publish DHT data:', err);
      } else {
        console.log('📤 Published DHT data:', dhtData);
      }
    });
  }, 5000);

  // Simulate LDR sensor data every 7 seconds
  setInterval(() => {
    const ldrData = {
      light_level: Math.floor(Math.random() * 4096), // 0-4095
      voltage: (Math.random() * 3.3).toFixed(2),
      light_status: Math.floor(Math.random() * 3),   // 0=Dark, 1=Dim, 2=Bright
      timestamp: Date.now()
    };
    
    client.publish(sensorTopic, JSON.stringify(ldrData), (err) => {
      if (err) {
        console.error('❌ Failed to publish LDR data:', err);
      } else {
        console.log('📤 Published LDR data:', ldrData);
      }
    });
  }, 7000);
});

client.on('error', (error) => {
  console.error('❌ MQTT connection error:', error);
});

client.on('message', (topic, message) => {
  if (topic === ledTopic) {
    console.log(`💡 Received LED control: ${message.toString()}`);
  }
});

client.on('disconnect', () => {
  console.log('🔌 Disconnected from MQTT broker');
});

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('\n🛑 Shutting down MQTT test...');
  client.end();
  process.exit(0);
});

console.log('📡 MQTT test running. Press Ctrl+C to stop.');
console.log(`📤 Publishing to: ${sensorTopic}`);
console.log(`📥 Listening on: ${ledTopic}`); 