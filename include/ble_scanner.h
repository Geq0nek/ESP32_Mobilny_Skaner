#ifndef BLE_SCANNER_H
#define BLE_SCANNER_H

#include "esp_err.h"
#include "host/ble_hs.h"

#define MAX_DEVICES 50

typedef struct {
    uint8_t addr[6]; // MAC address
    char name[32]; // Device name
    int8_t rssi; // Received Signal Strength Indicator
    double distance; // Estimated distance in [meters]
} ble_device_t;

/**
 * @brief Initialize the BLE scanner
 * @param interval Scan interval in milliseconds
 * @param window Scan window in milliseconds
 * @param duration_ms Scan duration in milliseconds
 * @return esp_err_t ESP_OK if successful, otherwise an error code
 */
esp_err_t ble_scanner_init(uint32_t interval, uint32_t window, uint32_t duration_ms);

/**
 * @brief Start BLE scan and collect results
 * @param results Pointer to an array of ble_device_t to store scan results
 * @param device_count Pointer to store the number of devices found
 * @return esp_err_t ESP_OK if successful, otherwise an error code
 */
esp_err_t ble_scan_start(ble_device_t **results, int *device_count);

/**
 * @brief function to print the results of the BLE scan
 */
void ble_print_results(ble_device_t *devices, int count);

/**
 * @brief Callback function for BLE GAP events
 * @param event Pointer to the BLE GAP event structure
 * @param arg User-defined argument (not used)
 * @return int 0 on success, non-zero on error
 */
int ble_gap_event_cb(struct ble_gap_event *event, void *arg);


/**
 * @brief Update BLE scan parameters
 * @param interval Scan interval in milliseconds
 * @param window Scan window in milliseconds
 * @param duration_ms Scan duration in milliseconds
 */
void ble_update_scan_params(uint32_t interval, uint32_t window, uint32_t duration_ms);


#endif // BLE_SCANNER_H


