idf_component_register(
    SRCS "ds18b20.c"
    INCLUDE_DIRS "."
    REQUIRES driver freertos esp_rom
)
