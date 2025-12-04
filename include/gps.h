#ifndef GPS_H
#define GPS_H

#include "driver/i2c.h"
#include "driver/uart.h"

#define UART_NUM               UART_NUM_1
#define BUF_SIZE               1024
#define UART_TX_PIN            13
#define UART_RX_PIN            12


typedef struct {
    char time[16];
    double lat;
    double lon;
    double alt;
    double speed;
} gps_data_t;

/**
 * @brief Initialize UART for GPS communication
 * This function configures the UART settings for GPS data reception.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t uart_gps_init();


/**
 * @brief Parse GNGGA sentence to extract latitude, longitude, altitude
 * @param line Pointer to the GNGGA sentence string     
 * @param lat Pointer to store latitude
 * @param lat_hemi Pointer to store latitude hemisphere ('N' or 'S')
 * @param lon Pointer to store longitude
 * @param lon_hemi Pointer to store longitude hemisphere ('E' or 'W')
 * @param alt Pointer to store altitude
 * @return true if parsing was successful, false otherwise
 */
bool parse_gngga(const char *line, char *lat, char *lat_hemi, char *lon, char *lon_hemi, char *alt, char *time);


/**
 * @brief Parse GNVTG sentence to extract speed
 * @param line Pointer to the GNVTG sentence string
 * @param speed Pointer to store speed
 * @return true if parsing was successful, false otherwise
 */
bool parse_gnvtg(const char *line, char *speed);


/**
 * @brief Convert NMEA latitude/longitude to decimal format
 * @param nmea NMEA formatted latitude/longitude string 
 * @param hemi Hemisphere indicator ('N', 'S', 'E', 'W')
 * @return double Decimal representation of latitude/longitude
 */
double nmea_to_decimal(const char *nmea, char hemi);

#endif