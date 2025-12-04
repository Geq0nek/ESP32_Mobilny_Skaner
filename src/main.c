#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi_scanner.h"
#include "ble_scanner.h"
#include "https_send.h"
#include "json_builder.h"
#include "gps.h"
#include "mqtt.h"
#include <ssd1306.h>
#include <time.h>
#include "esp_sntp.h"

#define WIIF_SSID "PLAY_Swiatlowodowy_99DF"
#define WIFI_PASSWORD "9Zg@7WNB1tcM"

// #define WIIF_SSID "iPhone (Tomasz)"
// #define WIFI_PASSWORD "goats2025"

#define GPS_LINE_MAX_LEN 128
#define GPS_QUEUE_SIZE 20

extern i2c_ssd1306_handle_t i2c_ssd1306;
extern int frequency;
extern int scan_interval;
extern int scan_window;

volatile gps_data_t gps_data = {0};
volatile int wifi_devices_counter = 0;
volatile int ble_devices_counter = 0;

const char *url = "https://esp32scannerwebserver.up.railway.app//api/receive/";
// const char *url = "http://192.168.0.68:8000/api/receive/";
const char *HTTPS_TAG = "HTTPS_SEND"; 

char *wifi_json_global = NULL;
char *ble_json_global = NULL;

bool wifi_json_ready = false;
bool ble_json_ready = false;

SemaphoreHandle_t json_mutex; 

QueueHandle_t gps_line_queue;

void wifi_scan_task(void *param)
{
    while (1) 
    {
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            wifi_scan_result_t *results = NULL;
            uint16_t found = 0;
            if (wifi_scan_start(&results, &found) == ESP_OK) 
            {
                wifi_print_results(results, found);
                char *wifi_json = json_build_wifi_scan_results(results, found);
                wifi_devices_counter = found;
                free(results);

                if (wifi_json != NULL) 
                {
                    ESP_LOGI("WIFI_JSON", "WiFi ble ready");
                }

                xSemaphoreTake(json_mutex, portMAX_DELAY);
                if (wifi_json_global) free(wifi_json_global);
                wifi_json_global = wifi_json;
                wifi_json_ready = true;
                xSemaphoreGive(json_mutex);
            }
        } else {
            ESP_LOGW("WIFI_SCAN", "Not connected to WiFi, skipping scan");
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void ble_scan_task(void *param)
{
    while (1) {
        ble_device_t *results = NULL;
        int count = 0;

        if (ble_scan_start(&results, &count) == ESP_OK) 
        {
            ble_print_results(results, count);
            char *json = json_build_ble_scan_results(results, count);
            ble_devices_counter = count;
            free(results);

            xSemaphoreTake(json_mutex, portMAX_DELAY);
            if (ble_json_global) free(ble_json_global);
            ble_json_global = json;
            ble_json_ready = true;
            xSemaphoreGive(json_mutex);

            ESP_LOGI("BLE", "BLE scan done: %d devices", count);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void gps_uart_task(void *param)
{
    static char nmea_buffer[2048];
    static int buffer_pos = 0;

    char uart_buf[512];

    while (1)
    {
        int len = uart_read_bytes(UART_NUM, (uint8_t *)uart_buf, sizeof(uart_buf) - 1, 100 / portTICK_PERIOD_MS);
        if (len > 0)
        {
            if (buffer_pos + len >= sizeof(nmea_buffer)) {
                buffer_pos = 0;
                ESP_LOGW("GPS", "NMEA buffer overflow, resetting");
            }

            memcpy(nmea_buffer + buffer_pos, uart_buf, len);
            buffer_pos += len;
            nmea_buffer[buffer_pos] = '\0';

            char *line_start = nmea_buffer;
            char *line_end;

            while ((line_end = strstr(line_start, "\r\n")) != NULL)
            {
                *line_end = '\0';

                char line_copy[GPS_LINE_MAX_LEN];
                strncpy(line_copy, line_start, GPS_LINE_MAX_LEN - 1);
                line_copy[GPS_LINE_MAX_LEN - 1] = '\0';

                if (xQueueSend(gps_line_queue, line_copy, 0) != pdTRUE) {
                    ESP_LOGW("GPS_QUEUE", "Queue full, dropping line");
                }

                line_start = line_end + 2;
            }

            int remaining = buffer_pos - (line_start - nmea_buffer);
            memmove(nmea_buffer, line_start, remaining);
            buffer_pos = remaining;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void gps_parse_task(void *param)
{
    char line[GPS_LINE_MAX_LEN];
    char lat[20], lon[20], alt[20], speed[20], time[16];
    char lat_hemi = 'N', lon_hemi = 'E';

    while (1)
    {
        if (xQueueReceive(gps_line_queue, line, portMAX_DELAY) == pdTRUE)
        {
            ESP_LOGI("GPS_LINE", "%s", line);

            if (strstr(line, "$GNGGA") != NULL) {
                ESP_LOGI("RAW_GGA", "GGA Line: %s", line);
            }

            if (parse_gngga(line, lat, &lat_hemi, lon, &lon_hemi, alt, time))
            {
                ESP_LOGI("GGA_PARSED", "lat: %s %c, lon: %s %c, alt: %s, time: %s", lat, lat_hemi, lon, lon_hemi, alt, time);

                gps_data.lat = nmea_to_decimal(lat, lat_hemi);
                gps_data.lon = nmea_to_decimal(lon, lon_hemi);
                gps_data.alt = atof(alt);
                strncpy((char*)gps_data.time, time, sizeof(gps_data.time));

                ESP_LOGI("DECIMAL", "lat: %f, lon: %f", gps_data.lat, gps_data.lon);
            }

            if (parse_gnvtg(line, speed)) {
                gps_data.speed = atof(speed);
                ESP_LOGI("SPEED", "speed: %s km/h → %f", speed, gps_data.speed);
            }
        }
    }
}

void json_merge_task(void *param)
{
    while (1) 
    {
        xSemaphoreTake(json_mutex, portMAX_DELAY);
        if (wifi_json_global && ble_json_global) 
        {
            char *full_json = json_build_scan_results(wifi_json_global, ble_json_global);
            if (full_json) 
            {
                free(full_json);
            }
        }
        xSemaphoreGive(json_mutex);
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}

void json_send_data(void *param)
{
    while (1) 
    {
        xSemaphoreTake(json_mutex, portMAX_DELAY);
        if (wifi_json_ready && ble_json_ready && wifi_json_global && ble_json_global)
        {
            char *full_json = json_build_scan_results(wifi_json_global, ble_json_global);
            if (full_json) 
            {
                if (https_send_json(url, full_json) == ESP_OK)
                {
                    ESP_LOGI(HTTPS_TAG, "Data sent successfully");
                    free(wifi_json_global); wifi_json_global = NULL;
                    free(ble_json_global);  ble_json_global  = NULL;
                    wifi_json_ready = false;
                    ble_json_ready = false;
                }
                free(full_json);
            }
        }
        xSemaphoreGive(json_mutex);
        vTaskDelay(pdMS_TO_TICKS(frequency * 1000)); 
    }
}

void oled_display_task(void *param)
{
    char buf[32];
    time_t now;
    struct tm timeinfo;

    while (1) 
    {
        i2c_ssd1306_buffer_clear(&i2c_ssd1306);
        ssd1306_print_str(0, 0, "ESP32 WiFi & BLE SCANNER", false);

        time(&now);
        localtime_r(&now, &timeinfo);

        strftime(buf, sizeof(buf), "TIME: %H:%M:%S", &timeinfo);
        ssd1306_print_str(0, 18, buf, false);

        snprintf(buf, sizeof(buf), "WiFi: %d", wifi_devices_counter);
        ssd1306_print_str(0, 30, buf, false);

        snprintf(buf, sizeof(buf), "BLE: %d", ble_devices_counter);
        ssd1306_print_str(0, 42, buf, false);

        ssd1306_display();
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
}

void init_sntp(void)
{
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
}

void set_timezone(void)
{
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();
}

void app_main(void)
{
    // init_ssd1306();

    json_mutex = xSemaphoreCreateMutex();

    gps_line_queue = xQueueCreate(GPS_QUEUE_SIZE, GPS_LINE_MAX_LEN);
    if (!gps_line_queue) {
        ESP_LOGE("APP", "Failed to create GPS queue");
    }

    ESP_ERROR_CHECK(wifi_connect_sta(WIIF_SSID, WIFI_PASSWORD));
    ESP_ERROR_CHECK(mqtt_init());
    mqtt_wait_for_config(pdMS_TO_TICKS(10000));
    ESP_LOGI("parameters", "frequency: %d, scan_interval: %d, scan_window: %d", frequency, scan_interval, scan_window);
    ESP_ERROR_CHECK(ble_scanner_init(0x50, 0x30, 7000));

    init_sntp();
    set_timezone();
    ESP_ERROR_CHECK(uart_gps_init());

    xTaskCreatePinnedToCore(wifi_scan_task, "wifi_scan_task", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(ble_scan_task, "ble_scan_task", 4096, NULL, 5, NULL, 0);

    xTaskCreatePinnedToCore(gps_uart_task, "gps_uart_task", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(gps_parse_task, "gps_parse_task", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(json_merge_task, "json_merge_task", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(json_send_data, "json_send_data", 4096, NULL, 5, NULL, 1);
    // xTaskCreatePinnedToCore(oled_display_task, "oled_display_task", 4096, NULL, 5, NULL, 1);
}







