#include "messages.h"
#include <cstring>

Message::Message() {
        this->id = 0;
        this->sender = ModuleId_t::DISPATCHER;
        this->receiver = ModuleId_t::DISPATCHER;
        this->length = 0;
        this->payload = nullptr; 
    }


Message::Message(uint8_t msgId, ModuleId_t sender, ModuleId_t receiver, uint8_t length) {
    this->id = msgId;
    this->sender = sender;
    this->receiver = receiver;
    this->length = length;
    this->payload = (length > 0) ? new uint8_t[length] : nullptr;
}

Message::Message(uint8_t msgId, ModuleId_t sender, ModuleId_t receiver, const uint8_t* data, uint8_t length) {
    this->id = msgId;
    this->sender = sender;
    this->receiver = receiver;
    this->length = length;
    this->payload = (length > 0) ? new uint8_t[length] : nullptr;
    if (this->payload && data) {
        std::memcpy(this->payload, data, length);
    }
}

Message::Message(const Message& other) {
    this->id = other.id;
    this->sender = other.sender;
    this->receiver = other.receiver;
    this->length = other.length;
    this->payload = (length > 0) ? new uint8_t[length] : nullptr;
    if (payload && other.payload) {
        std::memcpy(payload, other.payload, length);
    }
}

Message& Message::operator=(const Message& other) {
    if (this != &other) {
        delete[] payload;
        this->id = other.id;
        this->sender = other.sender;
        this->receiver = other.receiver;
        this->length = other.length;
        this->payload = (length > 0) ? new uint8_t[length] : nullptr;
        if (this->payload && other.payload) {
            std::memcpy(this->payload, other.payload, this->length);
        }
    }
    return *this;
}

Message::~Message() {
    delete[] this->payload;
}

void Message::setPayload(const uint8_t* data, uint8_t len) {
    delete[] this->payload;
    this->length = len;
    this->payload = (len > 0) ? new uint8_t[len] : nullptr;
    if (this->payload && data) {
        std::memcpy(this->payload, data, len);
    }
}
