idf_component_register(SRCS "TFT_eSPI.cpp"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES espressif__arduino-esp32 esp_driver_spi)

target_compile_options(
    ${COMPONENT_LIB}
    PRIVATE
        -Wno-error=missing-field-initializers
        -Wno-error=cpp)