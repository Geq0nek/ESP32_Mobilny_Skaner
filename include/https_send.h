#ifndef HTTPS_SEND_H
#define HTTPS_SEND_H

#include "esp_err.h"

 /**
  * @brief Sends JSON data to a specified HTTPS server.
  * @param url Full URL, e.g., "https://
  * @param json_data JSON string to send
  * @return esp_err_t ESP_OK on success, otherwise an error code
  */
esp_err_t https_send_json(const char *url, const char *json_data);


/** 
* @brief connects to a Wi-Fi network in STA mode
*
* @param ssid  SSID of the Wi-Fi network
* @param pass  Password for the Wi-Fi network (can be NULL for open networks)
* @return esp_err_t ESP_OK if successful, otherwise an error code
*/
esp_err_t wifi_connect_sta(const char *ssid, const char *pass);

#endif // HTTPS_SEND_H