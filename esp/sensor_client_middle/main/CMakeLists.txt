set(srcs "main.c")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS  "."
                    REQUIRES nvs_flash esp_wifi mqtt)
