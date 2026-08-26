set(APP_SRCS
    "main.c"

    "app/adafruit_io_service.c"

    "app/app_storage.c"
    "app/app_supervisor.c"
    "app/stack_profiler.c"

    "bacnet/bacnet_app.c"
    "bacnet/bacnet_coordinator.c"
    "bacnet/bacnet_event_bus.c"

    "bacnet/objects/analog_input.c"
    "bacnet/objects/analog_value.c"
    "bacnet/objects/binary_input.c"
    "bacnet/objects/binary_output.c"
    "bacnet/objects/binary_value.c"

    "platform/mstp_rs485.c"
    "platform/wifi_helper.c"

    "User_Settings.c"
)

if(NOT CONFIG_USER_DISPLAY_LORA_GATEWAY)
    list(APPEND APP_SRCS
        "app/sen54_bacnet_config.c"
        "app/sensor_service.c"
        "app/sensors/sen54_sensor_service.c"
    )
endif()

if(CONFIG_USER_TOUCH_CST816)
    list(APPEND APP_SRCS
        "ui/hardware/touch_cst816.c"
    )
endif()

if(CONFIG_USER_SENSOR_DS18B20)
    list(APPEND APP_SRCS
        "app/sensors/ds18b20_sensor_service.c"
    )
endif()

if(CONFIG_USER_DISPLAY_ST7796S)
    list(APPEND APP_SRCS
        "ui/profiles/display_st7796s.cpp"
    )

elseif(CONFIG_USER_DISPLAY_ST7796S_TEST)
    list(APPEND APP_SRCS
        "ui/profiles/display_st7796s_test.cpp"
    )

elseif(CONFIG_USER_DISPLAY_LVGL_TDISPLAY_S3)
    list(APPEND APP_SRCS
        "ui/profiles/display_lvgl_tdisplay_s3.cpp"
        "ui/lvgl/ui_port.c"
        "ui/lvgl/ui_theme.c"
        "ui/lvgl/ui_model.c"
        "ui/lvgl/ui_manager.c"
        "ui/lvgl/ui_screens.c"
        "ui/lvgl/screens/screen_measurements.c"
        "ui/lvgl/screens/screen_air_quality.c"
        "ui/lvgl/screens/screen_placeholder.c"
        "ui/lvgl/screens/screen_settings.c"
        "ui/lvgl/screens/screen_wifi.c"
        "ui/lvgl/screens/screen_wifi_password.c"
        "ui/lvgl/screens/screen_sensor_settings.c"
        "ui/lvgl/screens/screen_system.c"
    )

elseif(CONFIG_USER_DISPLAY_ST7789_GMT020)
    list(APPEND APP_SRCS
        "ui/profiles/display_st7789_gmt020.cpp"
    )

elseif(CONFIG_USER_DISPLAY_ST7789_HW657A)
    list(APPEND APP_SRCS
        "ui/profiles/display_st7789_hw657a.cpp"
    )

elseif(CONFIG_USER_DISPLAY_LORA_GATEWAY)
    list(APPEND APP_SRCS
        "ui/profiles/display_lora_gateway.c"
        "app/lora_gateway.c"
        "platform/lora_hal.c"
        "platform/sx1262.c"
    )

elseif(CONFIG_USER_DISPLAY_NONE)
    list(APPEND APP_SRCS
        "ui/profiles/display_none.c"
    )

elseif(NOT CMAKE_BUILD_EARLY_EXPANSION)
    message(FATAL_ERROR "No display profile selected")
endif()

set(REQUIRED_COMPONENTS
    bacnet-stack
    TFT_eSPI
    espressif__arduino-esp32
    ds18b20
)

if(NOT CONFIG_USER_DISPLAY_LORA_GATEWAY)
    list(APPEND REQUIRED_COMPONENTS
        sen54
    )
else()
    list(APPEND REQUIRED_COMPONENTS
        u8g2
        esp_driver_i2c
    )
endif()

if(CONFIG_USER_DISPLAY_LVGL_TDISPLAY_S3)
    list(APPEND REQUIRED_COMPONENTS
        lvgl
        esp_lvgl_port
    )
endif()

idf_component_register(
    SRCS
        ${APP_SRCS}

    INCLUDE_DIRS
        "."
        "app"
        "bacnet"
        "bacnet/objects"
        "platform"
        "ui"
        "ui/fonts"
        "ui/lvgl"
        "app/sensors"

    REQUIRES
        ${REQUIRED_COMPONENTS}

    PRIV_REQUIRES
        spi_flash
        nvs_flash
        esp_event
        esp_wifi
        esp_netif
        driver
        esp_driver_gpio
        esp_driver_spi
        esp_driver_uart
        esp_timer
        mqtt
        mbedtls
)

# Add linker flags for Arduino log wrapper compatibility
target_link_options(
    ${COMPONENT_LIB}
    INTERFACE
        "-Wl,--wrap=esp_log_write"
        "-Wl,--wrap=esp_log_writev"
)

# Force the linker to include Arduino log wrapper symbols
target_link_libraries(
    ${COMPONENT_LIB}
    INTERFACE
        "-Wl,--undefined=__wrap_esp_log_write"
        "-Wl,--undefined=__wrap_esp_log_writev"
)

set_source_files_properties(
    "ui/profiles/display_st7796s.cpp"
    "ui/profiles/display_st7796s_test.cpp"
    "ui/profiles/display_lvgl_tdisplay_s3.cpp"
    PROPERTIES
        COMPILE_OPTIONS "-Wno-error=cpp;-Wno-error=deprecated-enum-enum-conversion"
)