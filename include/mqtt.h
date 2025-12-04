#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "mqtt_client.h"
#include "esp_log.h"
#include "ble_scanner.h"

/**
 * @brief Initialize and configure the MQTT client
 * @return esp_err_t ESP_OK on success, otherwise an error code
 */
esp_err_t mqtt_init(void);

/**
 * @brief Wait for configuration parameters from MQTT broker
 * @param ticks_to_wait Maximum time to wait in ticks
 */
esp_err_t mqtt_wait_for_config(TickType_t ticks_to_wait);

#endif // MQTT_CLIENT_H
