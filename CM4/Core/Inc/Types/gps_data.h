#ifndef GPS_DATA_H
#define GPS_DATA_H

#include <stdint.h>

// Estructura para datos GPS
typedef struct {
    float latitude;
    float longitude;
    uint8_t fix;
} gpsData_t;

#endif // GPS_DATA_H
