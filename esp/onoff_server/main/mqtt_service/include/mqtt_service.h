#ifndef MQTT_SERVICE_H
#define MQTT_SERVICE_H

#include "esp_err.h"

/**
 * @brief Initialize MQTT service with TLS/SSL support
 * 
 * This function should only be called after WiFi has been initialized
 * and connected. Use mqtt_init_with_wifi() if you want to initialize
 * both WiFi and MQTT together.
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t mqtt_init(void);

/**
 * @brief Send a message to the MQTT broker
 * 
 * @param topic Topic to publish to
 * @param data Message data
 * @param len Length of data
 * @param qos QoS level (0, 1, or 2)
 * @param retain Whether to retain the message
 * @return esp_err_t ESP_OK on success
 */
esp_err_t mqtt_send_message(const char *topic, const char *data, int len, int qos, bool retain);

/**
 * @brief Subscribe to an MQTT topic
 * 
 * @param topic Topic to subscribe to
 * @param qos QoS level (0, 1, or 2)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t mqtt_subscribe_topic(const char *topic, int qos);

/**
 * @brief Register a callback for received MQTT messages
 * 
 * @param callback Function to be called when a message is received
 * @return esp_err_t ESP_OK on success
 */
typedef void (*mqtt_callback_t)(const char *topic, int topic_len, const char *data, int data_len);
esp_err_t mqtt_register_callback(mqtt_callback_t callback);

#endif /* MQTT_SERVICE_H */
