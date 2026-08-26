#include "lora_hal.h"

#include "board_pins.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define TAG "lora_hal"

static spi_device_handle_t lora_spi;
static SemaphoreHandle_t lora_event_sem;
static volatile uint32_t lora_isr_count;

static void IRAM_ATTR lora_dio1_isr(void *argument)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    (void)argument;
    lora_isr_count++;
    xSemaphoreGiveFromISR(lora_event_sem, &higher_priority_task_woken);
    if (higher_priority_task_woken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

void lora_hal_fem_set_rx(void)
{
    gpio_set_level(LORA_PA_POWER, 1);
    gpio_set_level(LORA_PA_EN, 1);
    gpio_set_level(LORA_PA_TX_EN, 0);
}

void lora_hal_fem_set_tx(void)
{
    gpio_set_level(LORA_PA_POWER, 1);
    gpio_set_level(LORA_PA_EN, 1);
    gpio_set_level(LORA_PA_TX_EN, 1);
}

void lora_hal_set_reset(uint32_t level)
{
    gpio_set_level(LORA_RESET, level);
}

esp_err_t lora_hal_wait_ready(TickType_t timeout)
{
    TickType_t start = xTaskGetTickCount();
    while (gpio_get_level(LORA_BUSY)) {
        if ((xTaskGetTickCount() - start) > timeout) {
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    return ESP_OK;
}

esp_err_t lora_hal_transfer(const uint8_t *tx, uint8_t *rx, size_t length)
{
    spi_transaction_t transaction = {
        .length = length * 8,
        .tx_buffer = tx,
        .rx_buffer = rx,
    };
    return spi_device_transmit(lora_spi, &transaction);
}

void lora_hal_clear_event(void)
{
    xSemaphoreTake(lora_event_sem, 0);
}

bool lora_hal_wait_event(TickType_t timeout)
{
    return xSemaphoreTake(lora_event_sem, timeout) == pdTRUE;
}

int lora_hal_dio1_level(void)
{
    return gpio_get_level(LORA_DIO1);
}

uint32_t lora_hal_dio1_isr_count(void)
{
    return lora_isr_count;
}

esp_err_t lora_hal_init(void)
{
    gpio_config_t output = {
        .pin_bit_mask = (1ULL << LORA_RESET) | (1ULL << LORA_PA_POWER) | (1ULL << LORA_PA_EN) | (1ULL << LORA_PA_TX_EN),
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&output), TAG, "reset GPIO setup failed");
    lora_hal_fem_set_rx();

    gpio_config_t input = {
        .pin_bit_mask = (1ULL << LORA_BUSY) | (1ULL << LORA_DIO1),
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_POSEDGE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&input), TAG, "radio GPIO setup failed");

    lora_event_sem = xSemaphoreCreateBinary();
    ESP_RETURN_ON_FALSE(lora_event_sem != NULL, ESP_ERR_NO_MEM, TAG, "event semaphore creation failed");
    ESP_RETURN_ON_ERROR(gpio_install_isr_service(ESP_INTR_FLAG_IRAM), TAG, "GPIO ISR service setup failed");
    ESP_RETURN_ON_ERROR(gpio_isr_handler_add(LORA_DIO1, lora_dio1_isr, NULL), TAG, "DIO1 ISR setup failed");

    spi_bus_config_t bus = {
        .mosi_io_num = LORA_MOSI,
        .miso_io_num = LORA_MISO,
        .sclk_io_num = LORA_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 64,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO), TAG, "SPI setup failed");

    spi_device_interface_config_t device = {
        .clock_speed_hz = 8000000,
        .mode = 0,
        .spics_io_num = LORA_NSS,
        .queue_size = 1,
    };
    ESP_RETURN_ON_ERROR(spi_bus_add_device(SPI2_HOST, &device, &lora_spi), TAG, "SPI device setup failed");
    return ESP_OK;
}
