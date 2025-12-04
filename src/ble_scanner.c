#include "ble_scanner.h"
#include "esp_log.h"
#include "distance.h"
#include "nvs_flash.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include <string.h>
#include <stdlib.h>

const char *TAG_BLE = "BLE_SCANNER";

static ble_device_t *scan_results = NULL;
static int scan_count = 0;
static bool scan_finished = false;
static uint8_t ble_addr_type;
static uint32_t scan_interval = 0x50;
static uint32_t scan_window   = 0x30;
static uint32_t scan_duration_ms = 5000;
static SemaphoreHandle_t scan_mutex;

volatile uint32_t scan_interval_new = 0;
volatile uint32_t scan_duration_new = 0;


int ble_gap_event_cb(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    case BLE_GAP_EVENT_DISC:
    {
        struct ble_hs_adv_fields fields;
        ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);

        xSemaphoreTake(scan_mutex, portMAX_DELAY);
        if (scan_count < MAX_DEVICES)
        {
            ble_device_t *dev = &scan_results[scan_count++];
            memcpy(dev->addr, event->disc.addr.val, 6);
            dev->rssi = event->disc.rssi;
            dev->distance = calculate_distance(event->disc.rssi);

            if (fields.name && fields.name_len > 0)
            {
                int len = fields.name_len < sizeof(dev->name) ? fields.name_len : sizeof(dev->name) - 1;
                memcpy(dev->name, fields.name, len);
                dev->name[len] = '\0';
            }
            else
            {
                strcpy(dev->name, "(no name)");
            }
        }
        xSemaphoreGive(scan_mutex);
        break;
    }
    case BLE_GAP_EVENT_DISC_COMPLETE:
        scan_finished = true;
        break;

    default:
        break;
    }
    return 0;
}

void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &ble_addr_type);
    ESP_LOGI(TAG_BLE, "BLE ready");
}

static void host_task(void *param)
{
    nimble_port_run();
}

esp_err_t ble_scanner_init(uint32_t interval, uint32_t window, uint32_t duration_ms)
{
    scan_interval = interval;
    scan_window = window;
    scan_duration_ms = duration_ms;

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(nimble_port_init());

    ble_svc_gap_init();
    ble_svc_gap_device_name_set("ESP32 BLE Scanner");

    ble_hs_cfg.sync_cb = ble_app_on_sync;

    scan_mutex = xSemaphoreCreateMutex();
    nimble_port_freertos_init(host_task);

    return ESP_OK;
}

esp_err_t ble_scan_start(ble_device_t **results, int *device_count)
{
    *results = NULL;
    *device_count = 0;

    xSemaphoreTake(scan_mutex, portMAX_DELAY);
    scan_count = 0;
    if (scan_results != NULL)
    {
        free(scan_results);
    }
    scan_results = calloc(MAX_DEVICES, sizeof(ble_device_t));
    xSemaphoreGive(scan_mutex);

    if (!scan_results)
    {
        ESP_LOGE(TAG_BLE, "Memory allocation failed");
        return ESP_ERR_NO_MEM;
    }

    struct ble_gap_disc_params params = {
        .passive = 0,
        .filter_duplicates = 1,
        .itvl = scan_interval,
        .window = scan_window,
        .limited = 0,
        .filter_policy = 0
    };

    scan_finished = false;
    int rc = ble_gap_disc(ble_addr_type, scan_duration_ms / 10, &params, ble_gap_event_cb, NULL);
    if (rc != 0)
    {
        ESP_LOGE(TAG_BLE, "Scan start failed: %d", rc);
        return ESP_FAIL;
    }

    while (!scan_finished)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    xSemaphoreTake(scan_mutex, portMAX_DELAY);
    *results = calloc(scan_count, sizeof(ble_device_t));
    if (*results && scan_count > 0)
    {
        memcpy(*results, scan_results, scan_count * sizeof(ble_device_t));
        *device_count = scan_count;
    }
    xSemaphoreGive(scan_mutex);

    return ESP_OK;
}

void ble_update_scan_params(uint32_t interval, uint32_t window, uint32_t duration_ms) {
    scan_interval = interval;
    scan_window   = window;
    scan_duration_ms = duration_ms;

    ble_gap_disc_cancel();

    struct ble_gap_disc_params params = {0};
    params.passive = 0;
    params.filter_duplicates = 1;
    params.itvl = scan_interval;
    params.window = scan_window;
    params.limited = 0;
    params.filter_policy = 0;

    scan_finished = false;
    int rc = ble_gap_disc(ble_addr_type, scan_duration_ms / 10, &params, ble_gap_event_cb, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG_BLE, "Scan restart failed: %d", rc);
    }
}

void ble_print_results(ble_device_t *devices, int count)
{
    printf("Found %d BLE devices:\n", count);
    printf("----------------------------------------------------------\n");
    printf("  RSSI | Distance | Name             | MAC\n");
    printf("----------------------------------------------------------\n");

    for (int i = 0; i < count; i++)
    {
        printf(" %5d | %8.2f | %-16s | %02X:%02X:%02X:%02X:%02X:%02X\n",
            devices[i].rssi,
            devices[i].distance,
            devices[i].name,
            devices[i].addr[0], devices[i].addr[1], devices[i].addr[2],
            devices[i].addr[3], devices[i].addr[4], devices[i].addr[5]);
    }
    printf("----------------------------------------------------------\n");
}