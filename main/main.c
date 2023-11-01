/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <string.h>
#include <sys/param.h>
#include <stdlib.h>

#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <app_wifi.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_standard_types.h>

#include "http_get_request.h"
#include "led_driver.h"

#define GPIO_INPUT_PIN  CONFIG_BOARD_GPIO_BOOT

/* Control http task to run HTTP GET */
typedef enum {
    SWITCH_OFF = 0,             /** switch off */
    SWITCH_ON,                  /** switch on */
} rmker_switch_t;

static const char *TAG = "main";

static esp_rmaker_device_t *g_rmker_device_tempsensor;
static esp_rmaker_device_t *g_rmker_device_led;
static rmker_switch_t       g_get_weather_flag = SWITCH_OFF;

/* Report weather data to RainMaker cloud */
esp_err_t rmaker_report_data(weather_data_t *weather_data)
{
    if (weather_data == NULL) {
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_rmaker_param_update_and_report(
            esp_rmaker_device_get_param_by_type(g_rmker_device_tempsensor, ESP_RMAKER_PARAM_TEMPERATURE),
            esp_rmaker_float(weather_data->temp)));

    ESP_ERROR_CHECK(esp_rmaker_param_update_and_report(
            esp_rmaker_device_get_param_by_name(g_rmker_device_tempsensor, "天气"),
            esp_rmaker_str(weather_data->text)));

    ESP_ERROR_CHECK(esp_rmaker_param_update_and_report(
            esp_rmaker_device_get_param_by_name(g_rmker_device_tempsensor, "地区"),
            esp_rmaker_str(weather_data->name)));

    ESP_ERROR_CHECK(esp_rmaker_param_update_and_report(
            esp_rmaker_device_get_param_by_name(g_rmker_device_tempsensor, "风力"),
            esp_rmaker_str(weather_data->wind_class)));

    ESP_ERROR_CHECK(esp_rmaker_param_update_and_report(
            esp_rmaker_device_get_param_by_name(g_rmker_device_tempsensor, "湿度"),
            esp_rmaker_int(weather_data->rh)));

    /* Check the current weather text */
    if (strstr(weather_data->text, "雨") != NULL) {
        ESP_ERROR_CHECK(esp_rmaker_raise_alert("下雨了，注意带伞！"));
    }

    if (weather_data->temp >= 20) {
        ESP_ERROR_CHECK(esp_rmaker_raise_alert("高温天气！小心中暑！"));
    }

    return ESP_OK;
}

/* Callback to handle param updates received from the RainMaker cloud */
static esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
            const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (ctx) {
        ESP_LOGI(TAG, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }

    const char *device_name = esp_rmaker_device_get_name(device);
    const char *param_name = esp_rmaker_param_get_name(param);
    ESP_LOGI(TAG, "Device name: %s - Device param: %s", device_name, param_name);

    if (strcmp(param_name, "power") == 0) {
        ESP_LOGI(TAG, "Received value = %s for %s - %s",
                val.val.b? "true" : "false", device_name, param_name);
        led_set_switch(val.val.b);
    } else if (strcmp(param_name, "color") == 0) {
        ESP_LOGI(TAG, "Received value = %d for %s - %s",
                val.val.i, device_name, param_name);
        led_set_hue(val.val.i);
    } else if (strcmp(param_name, "获取最新天气") == 0) {
        ESP_LOGI(TAG, "Received value = %s for %s - %s",
                val.val.b? "true" : "false", device_name, param_name);
        g_get_weather_flag = SWITCH_ON;
    } else {
        /* Silently ignoring invalid params */
        return ESP_OK;
    }

    esp_rmaker_param_update_and_report(param, val);
    return ESP_OK;
}

static void http_test_task(void *pvParameters)
{
    while(1) {
        if (gpio_get_level(GPIO_INPUT_PIN) == 0 || g_get_weather_flag == SWITCH_ON) {
            /* Allocate weather data from heap */
            weather_data_t *weather_data = http_weather_data_create();
            /* Perform an HTTP GET request with the weather data */
            ESP_ERROR_CHECK(http_get_with_url(HTTP_GET_URL_ADDRESS, weather_data));
            /* Report weather data to rainmaker cloud */
            ESP_ERROR_CHECK(rmaker_report_data(weather_data));

            /* Set the LED mode based on the weather text */
            ESP_ERROR_CHECK(led_set_mode(weather_data->text));
            /* Release weather data resource */
            ESP_ERROR_CHECK(http_weather_data_delete(weather_data));

            /* Reset the flag */
            g_get_weather_flag = SWITCH_OFF;
            /* Eliminate button debounce*/
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        /* Eliminate button debounce*/
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    vTaskDelete(NULL);
}

static void led_task(void *pvParameters)
{
    /* Configure the LED strip and obtain a handle */
    led_strip_handle_t led_strip = led_create();

    while(1) {
        /* Run the LED animation based on LED configuration and weather data */
        ESP_ERROR_CHECK(led_animations_start(led_strip));
    }

    vTaskDelete(NULL);
}

void app_main(void)
{
    /* Initialize NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Initialize Wi-Fi. Note that, this should be called before esp_rmaker_node_init() */
    app_wifi_init();

    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP RainMaker Device", "Temperature Sensor");

    /* Create a device to show the current weather */ 
    g_rmker_device_tempsensor = esp_rmaker_temp_sensor_device_create("今日天气", NULL, 0);

    /* Create RainMaker parameter */
    esp_rmaker_param_t *next_param = esp_rmaker_param_create("地区", NULL, esp_rmaker_str(""), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_t *text_param = esp_rmaker_param_create("天气", NULL, esp_rmaker_str(""), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_t *wind_param = esp_rmaker_param_create("风力", NULL, esp_rmaker_str(""), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_t *rh_param   = esp_rmaker_param_create("湿度", NULL, esp_rmaker_int(0), PROP_FLAG_READ | PROP_FLAG_WRITE);
    /* Create a button to get the latest weather */
    esp_rmaker_param_t *get_param  = esp_rmaker_param_create("获取最新天气", NULL, esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);

    esp_rmaker_param_add_ui_type(get_param, ESP_RMAKER_UI_PUSHBUTTON);
    esp_rmaker_device_add_param(g_rmker_device_tempsensor, next_param);
    esp_rmaker_device_add_param(g_rmker_device_tempsensor, text_param);
    esp_rmaker_device_add_param(g_rmker_device_tempsensor, wind_param);
    esp_rmaker_device_add_param(g_rmker_device_tempsensor, rh_param);
    esp_rmaker_device_add_param(g_rmker_device_tempsensor, get_param);
    esp_rmaker_device_assign_primary_param(g_rmker_device_tempsensor, text_param);

    /* Assign RainMaker callback function */
    esp_rmaker_device_add_cb(g_rmker_device_tempsensor, write_cb, NULL);
    esp_rmaker_node_add_device(node, g_rmker_device_tempsensor);

    /* Create a device to control the LED */
    g_rmker_device_led = esp_rmaker_device_create("Light", NULL, NULL);

    /* Create RainMaker parameter */
    esp_rmaker_param_t *power_param = esp_rmaker_param_create("power", NULL, esp_rmaker_bool(true), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_t *color_param = esp_rmaker_param_create("color", NULL, esp_rmaker_int(0), PROP_FLAG_READ | PROP_FLAG_WRITE);

    esp_rmaker_param_add_ui_type(color_param, ESP_RMAKER_UI_HUE_SLIDER);

    esp_rmaker_device_add_param(g_rmker_device_led, power_param);
    esp_rmaker_device_add_param(g_rmker_device_led, color_param);
    esp_rmaker_device_assign_primary_param(g_rmker_device_led, power_param);

    /* Assign RainMaker callback function */
    esp_rmaker_device_add_cb(g_rmker_device_led, write_cb, NULL);
    esp_rmaker_node_add_device(node, g_rmker_device_led);

    /* Start the ESP RainMaker Agent */
    esp_rmaker_start();

    /* Use BLE provisioning */
    app_wifi_start(POP_TYPE_NONE);

    xTaskCreate(http_test_task, "http_test_task", 6 * 1024, NULL, 5, NULL);
    xTaskCreate(led_task, "led_task", 6 * 1024, NULL, 5, NULL);
}
