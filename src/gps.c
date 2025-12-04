#include "gps.h"
#include "esp_log.h"
#include <string.h>
#include <stdbool.h>



esp_err_t uart_gps_init() {
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    return ESP_OK;
}

bool parse_gngga(const char *line, char *lat, char *lat_hemi, char *lon, char *lon_hemi, char *alt, char *time)
{
    // $GNGGA,123456.00,5200.1234,N,02100.5678,E,1,08,0.9,123.4,M,...
    if (!line || !strstr(line, "$GNGGA")) return false;

    char buf[128];
    strncpy(buf, line, sizeof(buf));
    buf[sizeof(buf)-1] = 0;

    char *token = strtok(buf, ","); // $GNGGA
    int field = 0;

    while (token != NULL)
    {
        switch (field)
        {
            case 1: // czas UTC
                if (time) strncpy(time, token, 15), time[15] = 0;
                break;
            case 2: // latitude
                if (lat) strncpy(lat, token, 15), lat[15] = 0;
                break;
            case 3: // N/S
                if (lat_hemi) *lat_hemi = token[0];
                break;
            case 4: // longitude
                if (lon) strncpy(lon, token, 15), lon[15] = 0;
                break;
            case 5: // E/W
                if (lon_hemi) *lon_hemi = token[0];
                break;
            case 9: // altitude
                if (alt) strncpy(alt, token, 15), alt[15] = 0;
                break;
        }
        token = strtok(NULL, ",");
        field++;
    }

    return lat && lat[0] && lon && lon[0];
}

bool parse_gnvtg(const char *line, char *speed)
{
    if (!line || !strstr(line, "$GNVTG")) return false;

    char buf[128];
    strncpy(buf, line, sizeof(buf));
    buf[sizeof(buf)-1] = 0;

    char *token;
    int field = 0;
    token = strtok(buf, ",");

    // GNVTG: pole 7 to prędkość w km/h
    // $GNVTG,0.00,T,,M,0.00,N,0.00,K*4E
    // Pola: 0-$GNVTG, 1-0.00, 2-T, 3- , 4-M, 5-0.00, 6-N, 7-0.00, 8-K
    while (token != NULL)
    {
        if (field == 7) 
        {
            strncpy(speed, token, 19);
            speed[19] = 0;
            return true;
        }
        token = strtok(NULL, ",");
        field++;
    }
    return false;
}

double nmea_to_decimal(const char *nmea, char hemi)
{
    // NMEA: DDMM.MMMMM
    double val = atof(nmea);
    int deg = (int)(val / 100);
    double min = val - deg * 100;
    double dec = deg + min / 60.0;
    if (hemi == 'S' || hemi == 'W') dec = -dec;
    return dec;
}

