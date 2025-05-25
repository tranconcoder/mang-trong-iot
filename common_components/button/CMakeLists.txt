idf_component_register(SRCS "button.c" "button_obj.cpp"
                    INCLUDE_DIRS  "." "include"
                    REQUIRES esp_driver_gpio
                    PRIV_REQUIRES esp_timer)
