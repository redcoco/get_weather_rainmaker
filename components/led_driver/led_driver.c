/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include "esp_log.h"

#include "led_driver.h"

#define LED_STRIP_RMT_RES_HZ    (10 * 1000 * 1000)      /* 10MHz resolution, 1 tick = 0.1us */

/** 
 * @brief To set LED mode 
 */
typedef enum {
    LED_MODE_OFF = 1,            /** LED off */
    LED_MODE_BREATH,             /** LED breath mode */
    LED_MODE_FLOW,               /** LED flow mode */
} led_mode_t;

/** 
 * @brief To control LED RGB value 
 */
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    int8_t direction;
    uint8_t brightness;
    led_mode_t mode;
} led_data_t;

/* configure LED default data */
#define LED_CONFIG_DEFAULT() { \
    .red        = LED_DEFAULT_RED, \
    .green      = LED_DEFAULT_GREEN, \
    .blue       = LED_DEFAULT_BLUE, \
    .direction  = LED_DEFAULT_BREATH_DIRECTION, \
    .brightness = LED_DEFAULT_BRIGHTNESS, \
    .mode       = LED_DEFAULT_MODE \
}

static const char *TAG  = "led_driver";
led_data_t g_led_data = LED_CONFIG_DEFAULT();

/* Configure the LED strip and obtain a handle */
led_strip_handle_t led_create(void)
{
    /* LED strip general initialization, according to your led board design */
    led_strip_config_t strip_config = {
        .strip_gpio_num   = LED_STRIP_BLINK_GPIO,   // The GPIO that connected to the LED strip's data line
        .max_leds         = LED_STRIP_LED_NUMBERS,  // The number of LEDs in the strip,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,   // Pixel format of your LED strip
        .led_model        = LED_MODEL_WS2812,       // LED strip model
        .flags.invert_out = false,                  // whether to invert the output signal
    };

    /* LED strip backend configuration: RMT */
    led_strip_rmt_config_t rmt_config = {
        .clk_src        = RMT_CLK_SRC_DEFAULT,           // Different clock source can lead to different power consumption
        .resolution_hz  = LED_STRIP_RMT_RES_HZ,    // RMT counter clock frequency
        .flags.with_dma = false,                  // DMA feature is available on ESP target like ESP32-S3
    };

    /* LED Strip object handle */
    led_strip_handle_t led_strip;
    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Created LED strip object with RMT backend");
    } else {
        ESP_LOGE(TAG, "Failed to created LED strip object");
        return NULL;
    }

    return led_strip;
}

/* Convert hue to RGB */
void led_set_hue(uint16_t hue)
{
    /* Make sure input between 0 ~ 360 */
    if (hue > 360) {
        hue = 360;
    }

    /* Calculate the RGB color corresponding to the hue */
    if (hue < 120) {
        g_led_data.red   = 255 - hue * 2;
        g_led_data.green = hue * 2;
        g_led_data.blue  = 0;
    } else if (hue < 240) {
        g_led_data.red   = 0;
        g_led_data.green = 255 - (hue - 120) * 2;
        g_led_data.blue  = (hue - 120) * 2;
    } else {
        g_led_data.red   = (hue - 240) * 2;
        g_led_data.green = 0;
        g_led_data.blue  = 255 - (hue - 240) * 2;
    }
}

/* Perform a breathing animation on the LED strip */
static void led_animation_breath(led_strip_handle_t led_strip)
{
    uint8_t color_red   = g_led_data.red * g_led_data.brightness / 255;
    uint8_t color_green = g_led_data.green * g_led_data.brightness / 255;
    uint8_t color_blue  = g_led_data.blue * g_led_data.brightness / 255;

    for (int i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
        led_strip_set_pixel(led_strip, i, color_red, color_green, color_blue);
    }
    /* Refresh the strip to send data */
    esp_err_t err = led_strip_refresh(led_strip);

    if (err != ESP_OK) {
            ESP_LOGE(TAG, "Refresh LED Strip failed");
    }

    g_led_data.brightness += g_led_data.direction;

    if (g_led_data.brightness >= 255 || g_led_data.brightness == 0) {
        g_led_data.direction = -g_led_data.direction;
    }
}

/* Perform a flowing animation on the LED strip */
static void led_animation_flow(led_strip_handle_t led_strip)
{
    /* Length of LED flow animation */
    int led_len = 3;

    for (int i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
        led_strip_clear(led_strip);

        for (int j = 0; j < led_len; j++) {
            int position = i - j;
            if (position >= 0) {
                uint8_t tail_r = g_led_data.red - (j * (g_led_data.red / led_len));
                uint8_t tail_g = g_led_data.green - (j * (g_led_data.green / led_len));
                uint8_t tail_b = g_led_data.blue - (j * (g_led_data.blue / led_len));
                led_strip_set_pixel(led_strip, position, tail_r, tail_g, tail_b);
            }
        }

        led_strip_set_pixel(led_strip, i, g_led_data.red, g_led_data.green, g_led_data.blue);
        esp_err_t err = led_strip_refresh(led_strip);

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Refresh LED Strip failed");
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* Set the LED strip for cloudy weather */
static void led_animation_off(led_strip_handle_t led_strip)
{
    led_strip_clear(led_strip);
    vTaskDelay(pdMS_TO_TICKS(10));
}

/* Set the LED strip for sunny weather */
static void led_set_sunny()
{
    g_led_data.mode = LED_MODE_BREATH;
    g_led_data.red = 255;
    g_led_data.green = 153;
    g_led_data.blue = 18;
}

/* Set the LED strip for rainy weather */
static void led_set_rain()
{
    g_led_data.mode = LED_MODE_FLOW;
    g_led_data.red = 255;
    g_led_data.green = 18;
    g_led_data.blue = 18;
}

/* Set the LED strip for cloudy weather */
static void led_set_cloudy()
{
    g_led_data.mode = LED_MODE_FLOW;
    g_led_data.red = 100;
    g_led_data.green = 100;
    g_led_data.blue = 100;
}

/* Set the LED strip for snow weather */
static void led_set_snow()
{
    g_led_data.mode = LED_MODE_FLOW;
    g_led_data.red = 200;
    g_led_data.green = 200;
    g_led_data.blue = 200;
}

/* Set the LED strip for unknown weather */
static void led_set_unknown_weather()
{
    g_led_data.mode = LED_MODE_BREATH;
    g_led_data.red = 255;
    g_led_data.green = 0;
    g_led_data.blue = 0;
}

/* Switch the LED animation on/off based on power status */
void led_set_switch(bool power)
{
    if (power) {
        g_led_data.mode = LED_MODE_BREATH;
    } else {
        g_led_data.mode = LED_MODE_OFF;
    }
}

/* Set LED mode based on weather text data */
esp_err_t led_set_mode(const char* weather)
{
    if (!weather) {
        ESP_LOGE(TAG, "Weather is NULL");
        return ESP_FAIL;
    }

    if (strstr(weather, "晴") != NULL) {
        led_set_sunny();
    } else if (strstr(weather, "雨") != NULL) {
        led_set_rain();
    } else if (strstr(weather, "多云") != NULL) {
        led_set_cloudy();
    } else if (strstr(weather, "雪") != NULL) {
        led_set_snow();
    } else {
        ESP_LOGW(TAG, "This weather has not been set yet");
        led_set_unknown_weather();
    }

    return ESP_OK;
}

/* Run LED animations based on weather data */
esp_err_t led_animations_start(led_strip_handle_t led_strip)
{
    if (led_strip == NULL) {
        return ESP_FAIL;
    }

    /* Check LED mode */
    switch (g_led_data.mode) {
        case LED_MODE_BREATH:
            led_animation_breath(led_strip);
            break;
        case LED_MODE_FLOW:
            led_animation_flow(led_strip);
            break;
        case LED_MODE_OFF:
            led_animation_off(led_strip);
            break;
        default:
            ESP_LOGW(TAG, "Unknown MODE, switch to LED_OFF");
            g_led_data.mode = LED_MODE_OFF;
            break;
    }

    return ESP_OK;
}
