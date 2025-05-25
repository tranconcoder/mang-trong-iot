set(srcs "main.c"
        "board.c")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS  "."
                    REQUIRES nvs_flash esp_wifi mqtt esp_driver_gpio)
