set(APP_SRCS
    "main.c"

    "app/adafruit_io_service.c"

    "app/app_storage.c"
    "app/app_supervisor.c"
    "app/sen54_bacnet_config.c"
    "app/sensor_service.c"
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

if(CONFIG_USER_DISPLAY_ST7796S_CURRENT)
    list(APPEND APP_SRCS
        "ui/profiles/display_st7796s_current.cpp"
    )

elseif(CONFIG_USER_DISPLAY_ST7796S_TEST)
    list(APPEND APP_SRCS
        "ui/profiles/display_st7796s_test.cpp"
    )

elseif(CONFIG_USER_DISPLAY_NONE)
    list(APPEND APP_SRCS
        "ui/profiles/display_none.c"
    )

elseif(NOT CMAKE_BUILD_EARLY_EXPANSION)
    message(FATAL_ERROR "No display profile selected")
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

    REQUIRES
        bacnet-stack
        TFT_eSPI
        espressif__arduino-esp32
        sen54
        ds18b20

    PRIV_REQUIRES
        spi_flash
        nvs_flash
        esp_event
        esp_wifi
        esp_netif
        driver
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