/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "led_strip.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LED_STRIP_BLINK_GPIO                CONFIG_LED_GPIO_INPUT  /* LED GPIO assignment */
#define LED_STRIP_LED_NUMBERS               CONFIG_LED_COUNT      /* Numbers of the LED in the strip */

#define LED_DEFAULT_RED                     150                     /* LED default value */
#define LED_DEFAULT_GREEN                   150
#define LED_DEFAULT_BLUE                    150
#define LED_DEFAULT_BREATH_DIRECTION        1
#define LED_DEFAULT_BRIGHTNESS              0
#define LED_DEFAULT_MODE                    2

/**
 * @brief Configure the LED strip and obtain a handle for LED control.
 *
 * @return A handle to the LED strip for subsequent LED control operations.
 */
led_strip_handle_t led_create(void);

/**
 * @brief Convert hue to RGB and update LED values.
 *
 * @param[in] hue The hue value. Range: 0 ~ 360
 */
void led_set_hue(uint16_t hue);

/**
 * @brief Switch the LED animation on/off based on power status.
 *
 * @param[in] power A boolean value indicating whether to turn the LED animation on or off.
 */
void led_set_switch(bool power);

/**
 * @brief Set the LED mode based on weather text data.
 *
 * @param[in] weather_text The weather text data to determine the LED mode.
 * @return ESP_OK if the mode was set successfully, otherwise an error code.
 */
esp_err_t led_set_mode(const char* weather_text);

/**
 * @brief Run LED animations based on weather data.
 *
 * @param[in] led_strip The LED strip handle.
 */
esp_err_t led_animations_start(led_strip_handle_t led_strip);

#ifdef __cplusplus
} /* end of extern "C" */
#endif
