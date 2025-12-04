#ifndef WIFI_SCANNER_H
#define WIFI_SCANNER_H

#include "esp_wifi.h"
#include "esp_err.h"

#define MAXIMUM_AP 50

typedef struct {
    char ssid[33];
    int8_t rssi;
    double distance;
    uint8_t bssid[6];
    uint8_t channel;
    char auth[16];
} wifi_scan_result_t;

/**
 * @brief Scan Wi-Fi and displays the results
 *  
 * @return esp_err_t - ESP_OK if successful, otherwise an error code
 */
esp_err_t wifi_scan_start();

/**
 * @brief Changes wifi_auth_mode_t to a human-readable string
 */
const char* auth_mode_type(wifi_auth_mode_t auth_mode);


/**
 * @brief Print the results of the Wi-Fi scan
 * 
 * @param results Pointer to an array of Wi-Fi scan results
 * @param found Number of Wi-Fi access points found
 */
void wifi_print_results(const wifi_scan_result_t *results, uint16_t found);

#endif // WIFI_SCANNER_H
