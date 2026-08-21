#pragma once

#include "sdkconfig.h"

#if defined(CONFIG_USER_DISPLAY_ST7796S) || \
    defined(CONFIG_USER_DISPLAY_ST7796S_TEST)

#include "User_Setups/Setup_Project_ST7796S.h"

#elif defined(CONFIG_USER_DISPLAY_LVGL_TDISPLAY_S3)

#include "User_Setups/Setup_Project_ST7789_TDISPLAY_S3.h"

#elif defined(CONFIG_USER_DISPLAY_ST7789_GMT020)

#include "User_Setups/Setup_Project_ST7789_GMT020.h"


#elif defined(CONFIG_USER_DISPLAY_ST7789_HW657A)

#include "User_Setups/Setup_Project_ST7789_HW657A.h"

#elif defined(CONFIG_USER_DISPLAY_NONE)

/*
 * TFT_eSPI remains an unconditional dependency for now.
 * Include a valid setup so the component can compile.
 */
#include "User_Setups/Setup_Project_ST7796S.h"

#else

#error "No TFT_eSPI hardware setup selected"

#endif