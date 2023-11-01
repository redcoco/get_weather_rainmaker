/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#define HTTP_GET_URL_ADDRESS                CONFIG_HTTP_URL_ADDRESS
#define MAX_HTTP_OUTPUT_BUFFER              2048
#define WEATHER_DEFAULT_GET                 false
#define WEATHER_STRING_SIZE                 16

/**
 * @brief To store weather data 
 */
typedef struct {
    float temp;
    int rh;
    char *name;
    char *text;
    char *wind_class;
} weather_data_t;

/**
 * @brief Allocates memory for a weather data structure on the heap.
 *
 * This function allocates memory for a `weather_data_t` structure on the heap
 * and initializes its members. The allocated structure can be used to store
 * weather-related information.
 *
 * @return A pointer to the allocated `weather_data_t` structure on success, or
 *         NULL on memory allocation failure.
 */
weather_data_t *http_weather_data_create();

/**
 * @brief Deallocates memory used by a weather data structure.
 *
 * This function releases the memory allocated for a `weather_data_t` structure
 * and its associated members, including strings. It should be called when the
 * structure is no longer needed to prevent memory leaks.
 *
 * @param weather_data Pointer to the `weather_data_t` structure to be deallocated.
 *
 * @return ESP_OK if delete successful, otherwise an error code.
 */
esp_err_t http_weather_data_delete(weather_data_t *weather_data);

/**
 * @brief Perform an HTTP GET request to fetch weather data and parse the returned JSON data.
 *
 * This function fetches weather data from a remote server using an HTTP GET request and
 * processes the JSON response to extract relevant information.
 *
 * @param http_url      The URL to fetch weather data from.
 * @param weather_data  Pointer to a `weather_data_t` structure to store the parsed data.
 *
 * @return ESP_OK if the GET request and parsing were successful, otherwise an error code.
 */
esp_err_t http_get_with_url(const char* http_url, weather_data_t *weather_data);

#ifdef __cplusplus
} /* end of extern "C" */
#endif