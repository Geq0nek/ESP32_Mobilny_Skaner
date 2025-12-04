#include "https_send.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"
#include <string.h>

static const char *TAG = "HTTPS_SEND";
extern const uint8_t cert_pem_start[] asm("_binary_cert_pem_start");
extern const uint8_t cert_pem_end[] asm("_binary_cert_pem_end");


const char *certification_pem = (const char *)cert_pem_start;

esp_err_t wifi_connect_sta(const char *ssid, const char *pass)
{
    static bool wifi_initialized = false;
    if (!wifi_initialized) {
        ESP_ERROR_CHECK(nvs_flash_init());
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        esp_netif_create_default_wifi_sta();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        wifi_initialized = true;
    }

    wifi_config_t wifi_config = { 0 };
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    int retry = 0;
    while (retry < 20) {
        wifi_ap_record_t info;
        if (esp_wifi_sta_get_ap_info(&info) == ESP_OK) {
            ESP_LOGI("WIFI", "Connected to %s", info.ssid);
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        retry++;
    }
    ESP_LOGW("WIFI", "Failed to connect to %s", ssid);
    return ESP_FAIL;
}

esp_err_t https_send_json(const char *url, const char *json_data)
{
    esp_http_client_config_t config = {
        .url = url,
        .cert_pem = certification_pem,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json; charset=utf-8");
    esp_http_client_set_post_field(client, json_data, strlen(json_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTPS POST sent, status = %d", status);
    } else {
        ESP_LOGE(TAG, "HTTPS POST request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}