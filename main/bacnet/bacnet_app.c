#include "bacnet_app.h"
#include <string.h>
#include "User_Settings.h"
/* Project object creation functions */
#include "analog_input.h"
#include "analog_value.h"
#include "binary_input.h"
#include "binary_output.h"
#include "binary_value.h"
#include "bacnet/bacaddr.h"
#include "bacnet/bacenum.h"
#include "bacnet/basic/bbmd/h_bbmd.h"
/* BACnet-stack object functions */
#include "bacnet/basic/object/ai.h"
#include "bacnet/basic/object/av.h"
#include "bacnet/basic/object/bi.h"
#include "bacnet/basic/object/bo.h"
#include "bacnet/basic/object/bv.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/npdu/h_npdu.h"
#include "bacnet/basic/service/h_cov.h"
#include "bacnet/basic/service/h_iam.h"
#include "bacnet/basic/service/h_rp.h"
#include "bacnet/basic/service/h_rpm.h"
#include "bacnet/basic/service/h_whois.h"
#include "bacnet/basic/service/h_wp.h"
#include "bacnet/basic/service/s_iam.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/bip.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlmstp.h"
#include "bacnet/datalink/mstp.h"
#include "bacnet/npdu.h"
#include "bacnet_event_bus.h"
#include "bacnet_coordinator.h"
#include "bacnet_dispatcher_config.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/semphr.h"
#include "mstp_rs485.h"
#include "wifi_helper.h"

static const char *TAG = "bacnet_app";

static bool s_bip_ready = false;
static bool s_mstp_ready = false;

static bacnet_app_profile_callback_t s_profile_callback = NULL;
static SemaphoreHandle_t s_datalink_mutex = NULL;
static char s_datalink_bip[] = "bip";
static char s_datalink_mstp[] = "mstp";
static char *s_datalink_default = NULL;
static uint8_t s_mstp_rx_buffer[512];
static uint8_t s_mstp_tx_buffer[512];
static struct mstp_port_struct_t s_mstp_port;
static struct dlmstp_user_data_t s_mstp_user;
static struct dlmstp_rs485_driver s_mstp_rs485_driver = {
    .init = MSTP_RS485_Init,
    .send = MSTP_RS485_Send,
    .read = MSTP_RS485_Read,
    .transmitting = MSTP_RS485_Transmitting,
    .baud_rate = MSTP_RS485_Baud_Rate,
    .baud_rate_set = MSTP_RS485_Baud_Rate_Set,
    .silence_milliseconds = MSTP_RS485_Silence_Milliseconds,
    .silence_reset = MSTP_RS485_Silence_Reset
};
static volatile uint32_t s_mstp_pdu_count = 0;
static volatile uint32_t s_mstp_apdu_count = 0;
static volatile uint32_t s_mstp_rp_total = 0;
static volatile uint32_t s_mstp_wp_total = 0;
static float s_mstp_rp_last_value = 0.0f;
static uint64_t s_core_last_fast_tick_us = 0;
static uint64_t s_core_last_slow_tick_us = 0;
static uint32_t s_mstp_i_am_tick = 0;
static uint32_t s_mstp_diag_reset_tick = 0;

static void bacnet_register_with_bbmd(void);
static void bacnet_receive_task(void *pvParameters);
static void bacnet_mstp_receive_task(void *pvParameters);
static void bacnet_core_task(void *pvParameters);
static void bacnet_cov_task(void *pvParameters);
static void bacnet_process_frame_event(const bacnet_event_t *evt);
static void bacnet_dispatcher_tick_100ms(void);
static void bacnet_dispatcher_tick_1s(void);
static void profiled_handler_read_property(uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data);
static void profiled_handler_write_property(uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data);

static void bacnet_profile_notify(bacnet_app_profile_event_t event)
{
    if (s_profile_callback) {
        s_profile_callback(event);
    }
}

static void bacnet_datalink_lock(char *name)
{
    if (s_datalink_mutex) {
        xSemaphoreTake(s_datalink_mutex, portMAX_DELAY);
    }
#if BACNET_USE_DISPATCHER_CORE
    bacnet_coordinator_activate_link_name(name);
#else
    datalink_set(name);
#endif
}

static void bacnet_datalink_unlock(void)
{
    if (s_datalink_default) {
#if BACNET_USE_DISPATCHER_CORE
        bacnet_coordinator_activate_link_name(s_datalink_default);
#else
        datalink_set(s_datalink_default);
#endif
    }
    if (s_datalink_mutex) {
        xSemaphoreGive(s_datalink_mutex);
    }
}

static bool bacnet_mstp_init(void)
{
    MSTP_RS485_Init();

    memset(&s_mstp_port, 0, sizeof(s_mstp_port));
    memset(&s_mstp_user, 0, sizeof(s_mstp_user));

    s_mstp_user.RS485_Driver = &s_mstp_rs485_driver;
    s_mstp_port.UserData = &s_mstp_user;
    s_mstp_port.InputBuffer = s_mstp_rx_buffer;
    s_mstp_port.InputBufferSize = sizeof(s_mstp_rx_buffer);
    s_mstp_port.OutputBuffer = s_mstp_tx_buffer;
    s_mstp_port.OutputBufferSize = sizeof(s_mstp_tx_buffer);

    dlmstp_set_interface((const char *)&s_mstp_port);
    dlmstp_set_mac_address(USER_MSTP_MAC_ADDRESS);
    dlmstp_set_max_info_frames(USER_MSTP_MAX_INFO_FRAMES);
    dlmstp_set_max_master(USER_MSTP_MAX_MASTER);
    dlmstp_set_baud_rate(USER_MSTP_BAUD_RATE);
    dlmstp_slave_mode_enabled_set(false);

    return dlmstp_init((char *)&s_mstp_port);
}

static bool bacnet_wifi_connected_now(void)
{
    if (!USER_ENABLE_BACNET_IP) {
        return false;
    }

    wifi_ap_record_t ap_info = {0};
    return esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK;
}

esp_err_t bacnet_app_init(
    bacnet_app_profile_callback_t profile_callback)
{
    esp_err_t err;

    s_profile_callback = profile_callback;

    /*
     * Reset runtime state in case initialization is ever retried.
     */
    s_bip_ready = false;
    s_mstp_ready = false;
    s_datalink_default = NULL;

    if (!USER_ENABLE_BACNET_IP &&
        !USER_ENABLE_BACNET_MSTP) {
        ESP_LOGE(
            TAG,
            "No BACnet transport is enabled");

        return ESP_ERR_INVALID_STATE;
    }

    /*
     * The datalink mutex is required by the receive, dispatcher,
     * COV and I-Am paths.
     */
    if (s_datalink_mutex == NULL) {
        s_datalink_mutex = xSemaphoreCreateMutex();

        if (s_datalink_mutex == NULL) {
            ESP_LOGE(
                TAG,
                "Failed to create BACnet datalink mutex");

            return ESP_ERR_NO_MEM;
        }
    }

    bacnet_coordinator_init();

    /*
     * The dispatcher core cannot operate without its event queue.
     */
    if (!bacnet_event_bus_init(0)) {
        ESP_LOGE(
            TAG,
            "Failed to initialize BACnet event bus");

        return ESP_ERR_NO_MEM;
    }

    /*
     * Initialize BACnet/IP.
     */
    if (USER_ENABLE_BACNET_IP) {
        err = esp_netif_init();

        if (err != ESP_OK) {
            ESP_LOGE(
                TAG,
                "esp_netif_init failed: %s",
                esp_err_to_name(err));

            return err;
        }

        err = esp_event_loop_create_default();

        /*
         * ESP_ERR_INVALID_STATE means another part of the
         * application already created the default event loop.
         * That state is usable and is not fatal.
         */
        if (err != ESP_OK &&
            err != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(
                TAG,
                "Default event loop creation failed: %s",
                esp_err_to_name(err));

            return err;
        }

        wifi_init_sta();

        ESP_LOGI(
            TAG,
            "Initializing BACnet stack (B/IP)");

#if BACNET_USE_DISPATCHER_CORE
        bacnet_coordinator_activate_link(
            BACNET_LINK_BIP);
#else
        datalink_set(s_datalink_bip);
#endif

        if (!datalink_init(NULL)) {
            ESP_LOGE(
                TAG,
                "Failed to initialize BACnet/IP datalink");

            return ESP_FAIL;
        }

        s_bip_ready = true;

        bacnet_coordinator_set_link_ready(
            BACNET_LINK_BIP,
            true);

        bacnet_coordinator_select_active_link();

        bacnet_coordinator_set_active_preference(
            BACNET_LINK_BIP);

        bacnet_register_with_bbmd();

        ESP_LOGI(
            TAG,
            "BACnet/IP transport initialized");
    }

    /*
     * Initialize BACnet MS/TP.
     *
     * Although currently disabled, do not silently continue if it
     * is enabled later but cannot initialize.
     */
    if (USER_ENABLE_BACNET_MSTP) {
        ESP_LOGI(
            TAG,
            "Initializing BACnet MS/TP");

        if (!bacnet_mstp_init()) {
            ESP_LOGE(
                TAG,
                "Failed to initialize BACnet MS/TP");

            return ESP_FAIL;
        }

#if BACNET_USE_DISPATCHER_CORE
        bacnet_coordinator_activate_link(
            BACNET_LINK_MSTP);
#else
        datalink_set(s_datalink_mstp);
#endif

        if (!datalink_init(
                (char *)&s_mstp_port)) {
            ESP_LOGE(
                TAG,
                "Failed to initialize BACnet MS/TP "
                "datalink interface");

            return ESP_FAIL;
        }

        s_mstp_ready = true;

        bacnet_coordinator_set_link_ready(
            BACNET_LINK_MSTP,
            true);

        bacnet_coordinator_select_active_link();

        ESP_LOGI(
            TAG,
            "BACnet MS/TP transport initialized");
    }

    /*
     * Select a default only from transports that actually
     * initialized successfully.
     */
    if (s_bip_ready) {
        s_datalink_default = s_datalink_bip;
    } else if (s_mstp_ready) {
        s_datalink_default = s_datalink_mstp;
    }

    if (s_datalink_default == NULL) {
        ESP_LOGE(
            TAG,
            "No BACnet transport initialized successfully");

        return ESP_FAIL;
    }

#if BACNET_USE_DISPATCHER_CORE
    bacnet_coordinator_activate_link_name(
        s_datalink_default);
#else
    datalink_set(s_datalink_default);
#endif

    if (s_datalink_default == s_datalink_mstp) {
        bacnet_coordinator_set_active_preference(
            BACNET_LINK_MSTP);
    }

    /*
     * Initialize the BACnet Device object and services.
     */
    Device_Init(NULL);

    Device_Set_Object_Instance_Number(
        USER_BACNET_DEVICE_INSTANCE);

    Device_Set_Vendor_Identifier(260);

    Device_Object_Name_ANSI_Init(
        USER_BACNET_DEVICE_NAME);

    ESP_LOGI(
        TAG,
        "Registering BACnet service handlers");

    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_I_AM,
        handler_i_am_add);

    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_WHO_IS,
        handler_who_is);

    apdu_set_unrecognized_service_handler_handler(
        handler_unrecognized_service);

    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY,
        profiled_handler_read_property);

    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROP_MULTIPLE,
        handler_read_property_multiple);

    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_WRITE_PROPERTY,
        profiled_handler_write_property);

    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_SUBSCRIBE_COV,
        handler_cov_subscribe);

    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_SUBSCRIBE_COV_PROPERTY,
        handler_cov_subscribe_property);

    handler_cov_init();

    /*
     * Create application BACnet objects.
     */
    bacnet_create_analog_inputs();
    bacnet_create_analog_values();
    bacnet_create_binary_inputs();
    bacnet_create_binary_values();
    bacnet_create_binary_outputs_with_gpio_sync();

    /*
     * Send I-Am only on transports that initialized successfully.
     */
    ESP_LOGI(TAG, "Broadcasting I-Am");

    if (s_bip_ready) {
        bacnet_datalink_lock(s_datalink_bip);
        Send_I_Am(Handler_Transmit_Buffer);
        bacnet_datalink_unlock();
    }

    if (s_mstp_ready) {
        bacnet_datalink_lock(s_datalink_mstp);
        Send_I_Am(Handler_Transmit_Buffer);
        bacnet_datalink_unlock();
    }

    s_mstp_i_am_tick = 0;
    s_mstp_diag_reset_tick = 0;

    ESP_LOGI(
        TAG,
        "BACnet application initialized successfully");

    return ESP_OK;
}


static void bacnet_app_delete_task(
    TaskHandle_t *task_handle)
{
    if (task_handle == NULL ||
        *task_handle == NULL) {
        return;
    }

    vTaskDelete(*task_handle);
    *task_handle = NULL;
}


static void bacnet_app_cleanup_started_tasks(
    const bacnet_app_task_handle_refs_t *task_handles)
{
    if (task_handles == NULL) {
        return;
    }

    bacnet_app_delete_task(task_handles->bip_rx);
    bacnet_app_delete_task(task_handles->mstp_rx);
    bacnet_app_delete_task(task_handles->core);
    bacnet_app_delete_task(task_handles->cov);
}



esp_err_t bacnet_app_start(
    const bacnet_app_task_handle_refs_t *task_handles)
{
    BaseType_t task_result;

    if (task_handles == NULL) {
        ESP_LOGE(
            TAG,
            "BACnet task-handle references are NULL");

        return ESP_ERR_INVALID_ARG;
    }

    /*
     * Validate the required output-handle pointers before creating
     * any tasks.
     */
    if (s_bip_ready &&
        task_handles->bip_rx == NULL) {
        ESP_LOGE(
            TAG,
            "Missing B/IP receive-task handle reference");

        return ESP_ERR_INVALID_ARG;
    }

    if (s_mstp_ready &&
        task_handles->mstp_rx == NULL) {
        ESP_LOGE(
            TAG,
            "Missing MS/TP receive-task handle reference");

        return ESP_ERR_INVALID_ARG;
    }

#if BACNET_USE_DISPATCHER_CORE
    if (task_handles->core == NULL) {
        ESP_LOGE(
            TAG,
            "Missing BACnet core-task handle reference");

        return ESP_ERR_INVALID_ARG;
    }
#else
    if (task_handles->cov == NULL) {
        ESP_LOGE(
            TAG,
            "Missing BACnet COV-task handle reference");

        return ESP_ERR_INVALID_ARG;
    }
#endif

    /*
     * Create the central processing task first. Receive tasks must
     * not enqueue frames unless a consumer exists.
     */
#if BACNET_USE_DISPATCHER_CORE
    task_result = xTaskCreate(
        bacnet_core_task,
        "bacnet_core",
        20480,
        NULL,
        2,
        task_handles->core);

    if (task_result != pdPASS) {
        ESP_LOGE(
            TAG,
            "Failed to create bacnet_core task");

        *task_handles->core = NULL;
        return ESP_ERR_NO_MEM;
    }
#else
    task_result = xTaskCreate(
        bacnet_cov_task,
        "bacnet_cov",
        24576,
        NULL,
        4,
        task_handles->cov);

    if (task_result != pdPASS) {
        ESP_LOGE(
            TAG,
            "Failed to create bacnet_cov task");

        *task_handles->cov = NULL;
        return ESP_ERR_NO_MEM;
    }
#endif

    /*
     * Create the B/IP receive task only if B/IP initialized.
     */
    if (s_bip_ready) {
        task_result = xTaskCreate(
            bacnet_receive_task,
            "bacnet_rx",
            16384,
            NULL,
            5,
            task_handles->bip_rx);

        if (task_result != pdPASS) {
            ESP_LOGE(
                TAG,
                "Failed to create bacnet_rx task");

            *task_handles->bip_rx = NULL;
            bacnet_app_cleanup_started_tasks(
                task_handles);

            return ESP_ERR_NO_MEM;
        }
    }

    /*
     * Create the MS/TP receive task only if MS/TP initialized.
     */
    if (s_mstp_ready) {
        task_result = xTaskCreate(
            bacnet_mstp_receive_task,
            "bacnet_mstp_rx",
            12288,
            NULL,
            5,
            task_handles->mstp_rx);

        if (task_result != pdPASS) {
            ESP_LOGE(
                TAG,
                "Failed to create bacnet_mstp_rx task");

            *task_handles->mstp_rx = NULL;
            bacnet_app_cleanup_started_tasks(
                task_handles);

            return ESP_ERR_NO_MEM;
        }
    }

    ESP_LOGI(
        TAG,
        "BACnet runtime tasks started successfully");

    return ESP_OK;
}

void bacnet_app_maintenance_1s(void)
{
#if !BACNET_USE_DISPATCHER_CORE
    if (s_bip_ready) { {
        bacnet_datalink_lock(s_datalink_bip);
        datalink_maintenance_timer(1);
        bacnet_datalink_unlock();
    }
#endif

    if (s_mstp_ready) {
        if (++s_mstp_i_am_tick % 60 == 0) {
            bacnet_app_send_mstp_i_am();
        }
        if (++s_mstp_diag_reset_tick % 30 == 0) {
            bacnet_app_reset_mstp_diagnostics();
        }
    }
}

void bacnet_app_send_mstp_i_am(void)
{
    if (!s_mstp_ready) {
        return;
    }

    bacnet_datalink_lock(s_datalink_mstp);
    Send_I_Am(Handler_Transmit_Buffer);
    bacnet_datalink_unlock();
}

void bacnet_app_reset_mstp_diagnostics(void)
{
    MSTP_RS485_Rx_Bytes_Get_Reset();
    MSTP_RS485_Preamble_Counts_Get_Reset(NULL, NULL);
    s_mstp_pdu_count = 0;
    s_mstp_apdu_count = 0;
    s_mstp_rp_total = 0;
    s_mstp_wp_total = 0;
}

uint32_t bacnet_app_get_mstp_pdu_count(void)
{
    return s_mstp_pdu_count;
}

bool bacnet_app_wifi_connected(void)
{
    return bacnet_wifi_connected_now();
}

static void bacnet_receive_task(void *pvParameters)
{
    (void)pvParameters;
    BACNET_ADDRESS src = {0};
    static uint8_t rx_buffer[600];
    uint16_t pdu_len = 0;

    ESP_LOGI(TAG, "BACnet receive task started");

    while (1) {
        memset(&src, 0, sizeof(src));
        pdu_len = bip_receive(&src, rx_buffer, sizeof(rx_buffer), 100);
        if (pdu_len > 0) {
            bacnet_event_t evt = {0};
            evt.type = BACNET_EVENT_RX_FRAME_BIP;
            evt.link_id = BACNET_EVENT_LINK_BIP;
            evt.length = pdu_len;
            evt.timestamp_us = (uint64_t)esp_timer_get_time();
            evt.src = src;
            if (evt.length > BACNET_EVENT_FRAME_MAX) {
                evt.length = BACNET_EVENT_FRAME_MAX;
            }

            memcpy(evt.data.frame, rx_buffer, evt.length);

            if (!bacnet_event_bus_enqueue(&evt, 0)) {
                ESP_LOGD(TAG, "BIP frame enqueue dropped (queue full)");
            }

#if !BACNET_USE_DISPATCHER_CORE
            bacnet_profile_notify(BACNET_APP_PROFILE_BIP_RX);
            BACNET_ADDRESS orig_src = src;
            BACNET_ADDRESS dest = {0};
            BACNET_NPDU_DATA npdu_data = {0};
            int apdu_offset = bacnet_npdu_decode(
                rx_buffer, pdu_len, &dest, &src, &npdu_data);
            if (src.len == 0) {
                src = orig_src;
            }
            if (apdu_offset > 0 && apdu_offset < (int)pdu_len) {
                bacnet_datalink_lock(s_datalink_bip);
                apdu_handler(&src, &rx_buffer[apdu_offset], pdu_len - apdu_offset);
                bacnet_datalink_unlock();
                bacnet_profile_notify(BACNET_APP_PROFILE_BIP_RX);
            }
#endif
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void bacnet_mstp_receive_task(void *pvParameters)
{
    (void)pvParameters;
    BACNET_ADDRESS src = {0};
    static uint8_t rx_buffer[600];
    uint16_t pdu_len = 0;

    ESP_LOGI(TAG, "BACnet MS/TP receive task started");

    while (1) {
        memset(&src, 0, sizeof(src));
        pdu_len = dlmstp_receive(&src, rx_buffer, sizeof(rx_buffer), 0);
        if (pdu_len > 0) {
            bacnet_event_t evt = {0};
            evt.type = BACNET_EVENT_RX_FRAME_MSTP;
            evt.link_id = BACNET_EVENT_LINK_MSTP;
            evt.length = pdu_len;
            evt.timestamp_us = (uint64_t)esp_timer_get_time();
            evt.src = src;
            if (evt.length > BACNET_EVENT_FRAME_MAX) {
                evt.length = BACNET_EVENT_FRAME_MAX;
            }

            memcpy(evt.data.frame, rx_buffer, evt.length);

            if (!bacnet_event_bus_enqueue(&evt, 0)) {
                ESP_LOGD(TAG, "MSTP frame enqueue dropped (queue full)");
            }

#if !BACNET_USE_DISPATCHER_CORE
            bacnet_profile_notify(BACNET_APP_PROFILE_MSTP_RX);
            s_mstp_pdu_count++;
            BACNET_ADDRESS dest = {0};
            BACNET_NPDU_DATA npdu_data = {0};
            int apdu_offset = bacnet_npdu_decode(
                rx_buffer, pdu_len, &dest, &src, &npdu_data);
            if (apdu_offset > 0 && apdu_offset < (int)pdu_len) {
                s_mstp_apdu_count++;
                if ((apdu_offset + 4) <= (int)pdu_len) {
                    uint8_t pdu_type = rx_buffer[apdu_offset] & 0xF0;
                    uint8_t service_choice = rx_buffer[apdu_offset + 3];
                    if (pdu_type == PDU_TYPE_CONFIRMED_SERVICE_REQUEST &&
                        service_choice == SERVICE_CONFIRMED_READ_PROPERTY) {
                        s_mstp_rp_total++;
                        s_mstp_rp_last_value = Analog_Value_Present_Value(1);
                    } else if (pdu_type == PDU_TYPE_CONFIRMED_SERVICE_REQUEST &&
                        service_choice == SERVICE_CONFIRMED_WRITE_PROPERTY) {
                        s_mstp_wp_total++;
                    }
                }
                bacnet_datalink_lock(s_datalink_mstp);
                apdu_handler(&src, &rx_buffer[apdu_offset], pdu_len - apdu_offset);
                bacnet_datalink_unlock();
                bacnet_profile_notify(BACNET_APP_PROFILE_MSTP_RX);
            } else {
                ESP_LOGW(TAG, "MS/TP RX frame decode failed: len=%u apdu_offset=%d src.len=%u src.mac=%u",
                    (unsigned)pdu_len, apdu_offset, (unsigned)src.len,
                    (unsigned)(src.len ? src.mac[0] : 0));
            }
#endif
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static void bacnet_cov_task(void *pvParameters)
{
    (void)pvParameters;
    while (1) {
        char *active_datalink = s_datalink_default;
        if (!active_datalink) {
            if (USER_ENABLE_BACNET_IP) {
                active_datalink = s_datalink_bip;
            } else if (USER_ENABLE_BACNET_MSTP) {
                active_datalink = s_datalink_mstp;
            }
        }

        if (active_datalink) {
            bacnet_datalink_lock(active_datalink);
            handler_cov_timer_seconds(1);
            bacnet_profile_notify(BACNET_APP_PROFILE_COV_NOTIFICATION);
            handler_cov_task();
            bacnet_profile_notify(BACNET_APP_PROFILE_COV_NOTIFICATION);
            bacnet_datalink_unlock();
        } else {
            handler_cov_timer_seconds(1);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void bacnet_register_with_bbmd(void)
{
    BACNET_IP_ADDRESS bbmd_addr = { { USER_BBMD_IP_OCTET_1, USER_BBMD_IP_OCTET_2,
                                     USER_BBMD_IP_OCTET_3, USER_BBMD_IP_OCTET_4 },
                                    USER_BBMD_PORT };
    int result = bvlc_register_with_bbmd(&bbmd_addr, USER_BBMD_TTL_SECONDS);
    ESP_LOGI(TAG, "BBMD register result: %d", result);
}

static void profiled_handler_read_property(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    bacnet_profile_notify(BACNET_APP_PROFILE_READ_PROPERTY);
    handler_read_property(service_request, service_len, src, service_data);
    bacnet_profile_notify(BACNET_APP_PROFILE_READ_PROPERTY);
}

static void profiled_handler_write_property(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    bacnet_profile_notify(BACNET_APP_PROFILE_WRITE_PROPERTY);
    handler_write_property(service_request, service_len, src, service_data);
    bacnet_profile_notify(BACNET_APP_PROFILE_WRITE_PROPERTY);
}

static void bacnet_process_frame_event(const bacnet_event_t *evt)
{
#if BACNET_USE_DISPATCHER_CORE
    if ((evt == NULL) || (evt->length == 0)) {
        return;
    }

    if (evt->link_id == BACNET_EVENT_LINK_BIP) {
        BACNET_ADDRESS src = evt->src;
        BACNET_ADDRESS orig_src = src;
        BACNET_ADDRESS dest = {0};
        BACNET_NPDU_DATA npdu_data = {0};
        int apdu_offset = bacnet_npdu_decode(
            (uint8_t *)evt->data.frame, evt->length, &dest, &src, &npdu_data);

        if (src.len == 0) {
            src = orig_src;
        }

        if (apdu_offset > 0 && apdu_offset < (int)evt->length) {
            bacnet_profile_notify(BACNET_APP_PROFILE_BIP_RX);
            bacnet_datalink_lock(s_datalink_bip);
            apdu_handler(&src, (uint8_t *)&evt->data.frame[apdu_offset], evt->length - apdu_offset);
            bacnet_datalink_unlock();
            bacnet_profile_notify(BACNET_APP_PROFILE_BIP_RX);
        }
    } else if (evt->link_id == BACNET_EVENT_LINK_MSTP) {
        BACNET_ADDRESS src = evt->src;
        BACNET_ADDRESS dest = {0};
        BACNET_NPDU_DATA npdu_data = {0};
        int apdu_offset = bacnet_npdu_decode(
            (uint8_t *)evt->data.frame, evt->length, &dest, &src, &npdu_data);
        if (apdu_offset > 0 && apdu_offset < (int)evt->length) {
            s_mstp_apdu_count++;
            if ((apdu_offset + 4) <= (int)evt->length) {
                uint8_t pdu_type = evt->data.frame[apdu_offset] & 0xF0;
                uint8_t service_choice = evt->data.frame[apdu_offset + 3];
                if (pdu_type == PDU_TYPE_CONFIRMED_SERVICE_REQUEST &&
                    service_choice == SERVICE_CONFIRMED_READ_PROPERTY) {
                    s_mstp_rp_total++;
                    s_mstp_rp_last_value = Analog_Value_Present_Value(1);
                } else if (pdu_type == PDU_TYPE_CONFIRMED_SERVICE_REQUEST &&
                    service_choice == SERVICE_CONFIRMED_WRITE_PROPERTY) {
                    s_mstp_wp_total++;
                }
            }

            bacnet_profile_notify(BACNET_APP_PROFILE_MSTP_RX);
            bacnet_datalink_lock(s_datalink_mstp);
            apdu_handler(&src, (uint8_t *)&evt->data.frame[apdu_offset], evt->length - apdu_offset);
            bacnet_datalink_unlock();
            bacnet_profile_notify(BACNET_APP_PROFILE_MSTP_RX);
        } else {
            ESP_LOGW(TAG, "MS/TP RX frame decode failed: len=%u apdu_offset=%d src.len=%u src.mac=%u",
                (unsigned)evt->length, apdu_offset, (unsigned)src.len,
                (unsigned)(src.len ? src.mac[0] : 0));
        }
    }
#else
    (void)evt;
#endif
}

static uint8_t dispatcher_bo1_last_state = BINARY_INACTIVE;

static void bacnet_dispatcher_tick_100ms(void)
{
    tsm_timer_milliseconds(100);

    if (USER_ENABLE_BACNET_IP) {
        bacnet_datalink_lock(s_datalink_bip);
        bacnet_profile_notify(BACNET_APP_PROFILE_COV_NOTIFICATION);
        handler_cov_task();
        bacnet_profile_notify(BACNET_APP_PROFILE_COV_NOTIFICATION);
        bacnet_datalink_unlock();
    }

    if (USER_ENABLE_BACNET_MSTP) {
        bacnet_datalink_lock(s_datalink_mstp);
        bacnet_profile_notify(BACNET_APP_PROFILE_COV_NOTIFICATION);
        handler_cov_task();
        bacnet_profile_notify(BACNET_APP_PROFILE_COV_NOTIFICATION);
        bacnet_datalink_unlock();
    }

    dispatcher_bo1_last_state = Binary_Output_Present_Value(USER_BO_INSTANCES[0]);
}

static void bacnet_dispatcher_tick_1s(void)
{
    handler_cov_timer_seconds(1);

    if (USER_ENABLE_BACNET_IP) {
        bacnet_datalink_lock(s_datalink_bip);
        datalink_maintenance_timer(1);
        bacnet_datalink_unlock();
    }
}

static void bacnet_core_task(void *pvParameters)
{
    (void)pvParameters;
    static bacnet_event_t evt = {0};

#if BACNET_USE_DISPATCHER_CORE
    s_core_last_fast_tick_us = (uint64_t)esp_timer_get_time();
    s_core_last_slow_tick_us = s_core_last_fast_tick_us;
    ESP_LOGI(TAG, "bacnet_core_task started (dispatcher mode)");
#else
    ESP_LOGI(TAG, "bacnet_core_task started (shadow mode)");
#endif

    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "bacnet_core_task entering main loop");

    while (1) {
        if (bacnet_event_bus_dequeue(&evt, pdMS_TO_TICKS(100))) {
#if BACNET_USE_DISPATCHER_CORE
            if ((evt.type == BACNET_EVENT_RX_FRAME_BIP) ||
                (evt.type == BACNET_EVENT_RX_FRAME_MSTP)) {
                if (evt.length > 0 && evt.length <= BACNET_EVENT_FRAME_MAX) {
                    bacnet_process_frame_event(&evt);
                } else {
                    ESP_LOGW(TAG, "dispatcher: invalid event length %u (max %u)",
                        (unsigned)evt.length, BACNET_EVENT_FRAME_MAX);
                }
            }
#else
            ESP_LOGD(TAG, "bacnet_core_task received event: type=%d len=%u",
                (int)evt.type, (unsigned)evt.length);
#endif
        }

#if BACNET_USE_DISPATCHER_CORE
        uint64_t now_us = (uint64_t)esp_timer_get_time();
        if ((now_us - s_core_last_fast_tick_us) >= 100000ULL) {
            s_core_last_fast_tick_us += 100000ULL;
            bacnet_dispatcher_tick_100ms();
        }
        if ((now_us - s_core_last_slow_tick_us) >= 1000000ULL) {
            s_core_last_slow_tick_us += 1000000ULL;
            bacnet_dispatcher_tick_1s();
        }
#endif
    }
}