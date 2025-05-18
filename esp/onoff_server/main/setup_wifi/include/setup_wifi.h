#ifndef SETUP_WIFI_H
#define SETUP_WIFI_H

#include "esp_err.h"

/**
 * @brief Initialize WiFi as station and connect to the specified access point
 * 
 * @param ssid WiFi SSID (can be NULL to use default from menuconfig)
 * @param password WiFi password (can be NULL to use default from menuconfig)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t setup_wifi_sta(const char *ssid, const char *password);

#endif // SETUP_WIFI_H
