
#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdint.h>

typedef struct {
    uint16_t id;         // ID del mensaje (2 bytes)
    uint8_t payload[8];  // Datos del mensaje (8 bytes)
} Messages_t;

#endif // MESSAGES_H