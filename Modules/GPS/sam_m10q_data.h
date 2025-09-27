#ifndef MODULES_GPS_SAM_M10Q_DATA_H_
#define MODULES_GPS_SAM_M10Q_DATA_H_

#include <cstdint>
#include <cstddef>

struct M10Q_ConfigEntry {
    const uint8_t* payload;
    uint8_t payload_len;
    const uint8_t checksum[2];
};

extern const M10Q_ConfigEntry m10q_config_table[];
extern const size_t m10q_config_table_size;

#endif /* MODULES_GPS_SAM_M10Q_DATA_H_ */
