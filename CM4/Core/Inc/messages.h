
#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdint.h>
#include "messages_id.h"

enum class ModuleId_t : uint8_t {
    SENSOR_ACQ   = 1,
    STIMULUS     = 2,
    GPS          = 3,
    LORA_TX      = 4,
    LORA_RX      = 5,
    FSM          = 6,
    DISTANCE     = 7,
    FENCE_UPDATE = 8,
    DISPATCHER   = 9
};

class Message {
public:
    uint8_t id;
    ModuleId_t sender;
    ModuleId_t receiver;
    uint8_t length;
    uint8_t* payload;

    Message();
    Message(uint8_t msgId, ModuleId_t sender, ModuleId_t receiver, uint8_t length);
    Message(uint8_t msgId, ModuleId_t sender, ModuleId_t receiver, const uint8_t* data, uint8_t length);
    Message(const Message& other);
    Message& operator=(const Message& other);
    ~Message();

    void setPayload(const uint8_t* data, uint8_t len);
};


#endif // MESSAGES_H