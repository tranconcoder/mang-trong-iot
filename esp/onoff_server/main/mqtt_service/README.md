# MQTT Service with TLS/SSL Support

This component provides secure MQTT connectivity with TLS/SSL support for ESP32-based BLE Mesh nodes.

## Features

- TLS/SSL encrypted connection to MQTT broker
- Username/password authentication
- Embedded root certificate for TLS verification
- Message publishing and subscription
- Callback-based message handling
- Integrated WiFi initialization and connection

## Usage

### Option 1: Initialize both WiFi and MQTT together (recommended)

```c
#include "mqtt_service.h"

void app_main(void)
{
    // Initialize WiFi and MQTT together
    esp_err_t err = mqtt_init_with_wifi("your-wifi-ssid", "your-wifi-password");
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize WiFi and MQTT");
        return;
    }

    // Register callback for received messages
    mqtt_register_callback(on_mqtt_message);

    // Subscribe to topics
    mqtt_subscribe_topic("my/topic", 0);

    // Publish message
    mqtt_send_message("status/topic", "Hello MQTT!", 11, 0, false);
}
```

### Option 2: Initialize WiFi separately and then MQTT

If you already have WiFi initialized in your application:

```c
#include "mqtt_service.h"

void app_main(void)
{
    // ... your WiFi initialization code ...

    // Initialize MQTT client with TLS/SSL
    esp_err_t err = mqtt_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize MQTT");
        return;
    }

    // Rest of the code is the same
    mqtt_register_callback(on_mqtt_message);
    mqtt_subscribe_topic("my/topic", 0);
    mqtt_send_message("status/topic", "Hello MQTT!", 11, 0, false);
}
```

### Message Callback

```c
// Callback function for received messages
void on_mqtt_message(const char *topic, int topic_len, const char *data, int data_len)
{
    ESP_LOGI(TAG, "Received message on topic: %.*s", topic_len, topic);
    ESP_LOGI(TAG, "Message: %.*s", data_len, data);

    // Process message...
}
```

## Configuration

The component has built-in configuration:

- MQTT Broker: fd66ecb3.ala.asia-southeast1.emqxsl.com
- Port: 8883 (TLS/SSL)
- Username: trancon
- Password: 123
- Default WiFi SSID: "your-ssid" (can be overridden)
- Default WiFi Password: "your-password" (can be overridden)
- Root certificate: DigiCert Global Root CA

To change these settings, modify the defines in `mqtt_service.c`:

```c
// MQTT settings
#define MQTT_HOST "fd66ecb3.ala.asia-southeast1.emqxsl.com"
#define MQTT_PORT 8883
#define MQTT_USERNAME "trancon"
#define MQTT_PASSWORD "123"

// Default WiFi settings
#define EXAMPLE_ESP_WIFI_SSID      "your-ssid"
#define EXAMPLE_ESP_WIFI_PASS      "your-password"
```
