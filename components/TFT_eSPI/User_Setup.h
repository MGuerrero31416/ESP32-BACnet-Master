#pragma once

#include "sdkconfig.h"

/*
 * Select the TFT_eSPI hardware configuration using the same
 * Kconfig option that selects the UI implementation.
 */

#if defined(CONFIG_USER_DISPLAY_ST7796S_CURRENT) || \
    defined(CONFIG_USER_DISPLAY_ST7796S_TEST)

#include "User_Setups/Setup_Project_ST7796S.h"

#else

#error "No TFT_eSPI hardware setup selected"

#endif