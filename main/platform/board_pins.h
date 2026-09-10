#pragma once

#include "driver/gpio.h"

// Heltec WiFi LoRa 32 V4 board wiring used by the SX1262 radio layer.
#define LORA_NSS GPIO_NUM_8
#define LORA_SCK GPIO_NUM_9
#define LORA_MOSI GPIO_NUM_10
#define LORA_MISO GPIO_NUM_11
#define LORA_RESET GPIO_NUM_12
#define LORA_BUSY GPIO_NUM_13
#define LORA_DIO1 GPIO_NUM_14

// GC1109 front-end module control.
#define LORA_PA_POWER GPIO_NUM_7
#define LORA_PA_EN GPIO_NUM_2
#define LORA_PA_TX_EN GPIO_NUM_46
