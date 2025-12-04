#include "json_builder.h"
#include "cJSON.h"

extern gps_data_t gps_data;

char* json_build_wifi_scan_results(wifi_scan_result_t *wifi_records, int count)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *wifi_array = cJSON_CreateArray();

    for (int i=0; i < count; i++)
    {
        cJSON *ap = cJSON_CreateObject();
        cJSON_AddStringToObject(ap, "ssid",(const char*)wifi_records[i].ssid);
        cJSON_AddNumberToObject(ap, "rssi", wifi_records[i].rssi);
        cJSON_AddNumberToObject(ap, "distance", wifi_records[i].distance);

        char mac[18];
        snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x",
                 wifi_records[i].bssid[0], wifi_records[i].bssid[1],
                 wifi_records[i].bssid[2], wifi_records[i].bssid[3],
                 wifi_records[i].bssid[4], wifi_records[i].bssid[5]);
        cJSON_AddStringToObject(ap, "mac", mac);
        cJSON_AddNumberToObject(ap, "channel", wifi_records[i].channel);
        cJSON_AddStringToObject(ap, "auth", wifi_records[i].auth);
        cJSON_AddItemToArray(wifi_array, ap);        
    }

    cJSON_AddItemToObject(root, "wifi", wifi_array);
    char *json_string = cJSON_PrintUnformatted(root);

    cJSON_Delete(root); 
    return json_string; 
}

char* json_build_ble_scan_results(ble_device_t *ble_devices, int count)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *ble_array = cJSON_CreateArray();

    for (int i=0; i < count; i++)
    {
        cJSON *device = cJSON_CreateObject();
        cJSON_AddStringToObject(device, "name", ble_devices[i].name);
        cJSON_AddNumberToObject(device, "rssi", ble_devices[i].rssi);

        char mac[18];
        snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x",
                 ble_devices[i].addr[0], ble_devices[i].addr[1],
                 ble_devices[i].addr[2], ble_devices[i].addr[3],
                 ble_devices[i].addr[4], ble_devices[i].addr[5]);
        cJSON_AddStringToObject(device, "mac", mac);
        cJSON_AddNumberToObject(device, "distance", (char) ble_devices[i].distance);
        cJSON_AddItemToArray(ble_array, device);
    }

    cJSON_AddItemToObject(root, "ble", ble_array);
    char *json_string = cJSON_PrintUnformatted(root);

    cJSON_Delete(root); 
    return json_string; 
}

char* json_build_scan_results(const char *wifi_json_str, const char *ble_json_str)
{
    cJSON *wifi_json = cJSON_Parse(wifi_json_str);
    cJSON *ble_json = cJSON_Parse(ble_json_str);

    if (!wifi_json || !ble_json) {
        return NULL; // Return NULL if parsing fails
    }

    cJSON *root = cJSON_CreateObject();

    cJSON *wifi_array = cJSON_GetObjectItem(wifi_json, "wifi");
    cJSON *ble_array = cJSON_GetObjectItem(ble_json, "ble");

    cJSON *gps = cJSON_CreateObject();
    cJSON_AddStringToObject(gps, "time", gps_data.time);
    cJSON_AddNumberToObject(gps, "lat", gps_data.lat);
    cJSON_AddNumberToObject(gps, "lon", gps_data.lon);
    cJSON_AddNumberToObject(gps, "alt", gps_data.alt);
    cJSON_AddNumberToObject(gps, "speed", gps_data.speed);
    cJSON_AddItemToObject(root, "gps", gps);
    ESP_LOGI("GPS", "lat=%.6f lon=%.6f", gps_data.lat, gps_data.lon);

    cJSON_AddItemToObject(root, "wifi", cJSON_Duplicate(wifi_array, 1));
    cJSON_AddItemToObject(root, "ble", cJSON_Duplicate(ble_array, 1));

    char *json_str = cJSON_PrintUnformatted(root);

    cJSON_Delete(wifi_json); 
    cJSON_Delete(ble_json); 
    cJSON_Delete(root);
    return json_str; 
}