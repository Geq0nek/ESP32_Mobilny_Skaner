#ifndef JSON_BUILDER_H
#define JSON_BUILDER_H

#include "wifi_scanner.h"
#include "ble_scanner.h"
#include "gps.h"

/**
 * @brief Build JSON string for Wi-Fi scan results
 * @param wifi_records Pointer to array of Wi-Fi access point records
 * @param count Number of Wi-Fi access points
 * @return JSON string containing Wi-Fi scan results
 */
char* json_build_wifi_scan_results(wifi_scan_result_t *wifi_records, int count);


/**
 * @brief Build JSON string for BLE scan results
 * @param ble_devices Pointer to array of BLE device records
 * @param count Number of BLE devices
 * @return JSON string containing BLE scan results
 */
char* json_build_ble_scan_results(ble_device_t *ble_devices, int count);


/**
 * @brief Build JSON string for GPS data
 * @param lat Latitude
 * @param lon Longitude
 * @param alt Altitude
 * @param speed Speed
 * @return JSON string containing GPS data
 */
char* json_build_gps_data(double lat, double lon, double alt, double speed);


/**
 * @brief Build JSON string containing both Wi-Fi and BLE scan results
 * @param wifi_json_str JSON object containing Wi-Fi scan results
 * @param ble_json_str JSON object containing BLE scan results
 * @return JSON string containing both Wi-Fi and BLE scan results
 */
char* json_build_scan_results(const char *wifi_json_str, const char *ble_json_str);



#endif // JSON_BUILDER_H