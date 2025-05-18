idf_component_register(SRCS "setup_wifi.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_wifi esp_event esp_netif nvs_flash freertos)
