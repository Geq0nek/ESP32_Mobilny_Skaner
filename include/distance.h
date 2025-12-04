#include <math.h>

#ifndef DISTANCE_H
#define DISTANCE_H

/**
 * @brief Calculate the distance based on RSSI
 * @param rssi Received Signal Strength Indicator
 * @return double Estimated distance in meters
 */
static inline float calculate_distance(int rssi) {
    double n = 2.89;
    double dbmref = -58.5;
    return pow(10, ((dbmref - rssi) / (10.0 * n)));
}

#endif // DISTANCE_H