idf_component_register(
    SRCS "ds18b20.c"
    INCLUDE_DIRS "."
    REQUIRES freertos esp_rom esp_driver_gpio
)
