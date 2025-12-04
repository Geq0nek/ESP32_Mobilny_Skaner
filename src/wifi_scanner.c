#include "wifi_scanner.h"
#include "nvs_flash.h"
#include "distance.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


const char* auth_mode_type(wifi_auth_mode_t auth_mode)
{
    static const char *types[] = {"OPEN", "WEP", "WPA PSK", "WPA2 PSK", "WPA WPA2 PSK", "MAX"};
    if (auth_mode < sizeof(types)/sizeof(types[0]))
        return types[auth_mode];
    return "UNKNOWN";
}

esp_err_t wifi_scan_start(wifi_scan_result_t **results, uint16_t *found)
{
    wifi_scan_config_t scan_config = {
        .ssid = 0,
        .bssid = 0,
        .channel = 0,
        .show_hidden = true
    };

    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));

    uint16_t ap_num = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_num));
    if (ap_num > MAXIMUM_AP) ap_num = MAXIMUM_AP;

    wifi_ap_record_t *wifi_records = malloc(sizeof(wifi_ap_record_t) * ap_num);
    if (!wifi_records) return ESP_ERR_NO_MEM;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, wifi_records));

    wifi_scan_result_t *list = malloc(sizeof(wifi_scan_result_t) * ap_num);
    if (!list) {
        free(wifi_records);
        return ESP_ERR_NO_MEM;
    }

    for (int i = 0; i < ap_num; i++) {
        strncpy(list[i].ssid, (char*)wifi_records[i].ssid, 32);
        list[i].ssid[32] = '\0';
        list[i].rssi = wifi_records[i].rssi;
        list[i].distance = calculate_distance(wifi_records[i].rssi);
        memcpy(list[i].bssid, wifi_records[i].bssid, 6);
        list[i].channel = wifi_records[i].primary;
        strncpy(list[i].auth, auth_mode_type(wifi_records[i].authmode), 15);
        list[i].auth[15] = '\0';
    }

    free(wifi_records);

    *results = list;
    *found = ap_num;
    return ESP_OK;
}

void wifi_print_results(const wifi_scan_result_t *results, uint16_t found)
{
    printf("Number of Access Points Found: %d\n", found);
    printf("\n");
    printf("               SSID              | Channel | RSSI |  Distance  | Authentication Mode |         MAC\n");
    printf("**********************************************************************************************\n");
    for (int i = 0; i < found; i++)
        printf("%32s | %7d | %4d | %8.2f | %16s | %02x:%02x:%02x:%02x:%02x:%02x\n",
            results[i].ssid,
            results[i].channel,
            results[i].rssi,
            results[i].distance,
            results[i].auth,
            results[i].bssid[0], results[i].bssid[1], results[i].bssid[2],
            results[i].bssid[3], results[i].bssid[4], results[i].bssid[5]);
    printf("**********************************************************************************************\n");
}