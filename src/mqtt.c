#include "mqtt.h"
#include "cJSON.h"

#define MQTT_CONFIG_RECEIVED_BIT BIT0

int frequency;
int scan_interval;
int scan_window;

static EventGroupHandle_t mqtt_event_group;

static const char *TAG = "MQTT";

static esp_mqtt_client_handle_t client = NULL;

static void mqtt_event_handler_cb(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch(event->event_id)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            esp_mqtt_client_subscribe(event->client, "esp32/config", 0);
            break;
        case MQTT_EVENT_DATA:
            if (event->topic_len == strlen("esp32/config") &&
                strncmp(event->topic, "esp32/config", event->topic_len) == 0) {

                char json_buf[256];
                int len = event->data_len < sizeof(json_buf) - 1 ? event->data_len : sizeof(json_buf) - 1;
                memcpy(json_buf, event->data, len);
                json_buf[len] = '\0';

                cJSON *root = cJSON_Parse(json_buf);
                if (root) {
                    cJSON *freq = cJSON_GetObjectItem(root, "frequency");
                    cJSON *time = cJSON_GetObjectItem(root, "time_interval");
                    cJSON *scan = cJSON_GetObjectItem(root, "scan_interval");
                    if (cJSON_IsNumber(freq)) frequency = freq->valueint;
                    if (cJSON_IsNumber(time)) scan_interval = time->valueint;
                    if (cJSON_IsNumber(scan)) scan_window = scan->valueint;

                    uint16_t ble_scan_interval = scan_interval * 1600; 
                    uint16_t ble_scan_window   = scan_window * 1600;
                    
                    ble_update_scan_params(ble_scan_interval, ble_scan_window, 7000);
                    ESP_LOGI(TAG, "frequency=%d, time_interval=%d, scan_interval=%d", frequency, scan_interval, scan_window);
                    xEventGroupSetBits(mqtt_event_group, MQTT_CONFIG_RECEIVED_BIT);

                    cJSON_Delete(root);
                } else {
                    ESP_LOGW(TAG, "JSON parse error");
                }
            }
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        default:
            break;
    }
}

esp_err_t mqtt_init(void)
{
    mqtt_event_group = xEventGroupCreate();
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://broker.emqx.io:1883",
        // .broker.address.username = "user",                  
        // .broker.address.password = "password",              
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to init MQTT client");
        return ESP_FAIL;
    }

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler_cb, NULL);
    esp_mqtt_client_start(client);

    return ESP_OK;
}

esp_err_t mqtt_wait_for_config(TickType_t timeout_ticks)
{
    if (!mqtt_event_group)
    {
        ESP_LOGW(TAG, "MQTT event group not initialized, using defaults");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Waiting for MQTT configuration...");
    EventBits_t bits = xEventGroupWaitBits(
        mqtt_event_group,
        MQTT_CONFIG_RECEIVED_BIT,
        pdFALSE,
        pdFALSE,
        timeout_ticks
    );

    if (bits & MQTT_CONFIG_RECEIVED_BIT)
    {
        ESP_LOGI(TAG, "MQTT configuration received!");
        return ESP_OK;
    }
    else
    {
        ESP_LOGW(TAG, "Timeout waiting for MQTT configuration, using default parameters");
        return ESP_ERR_TIMEOUT;
    }
}
