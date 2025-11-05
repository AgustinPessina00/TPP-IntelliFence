#ifndef MESSAGE_WRAPPER_HPP
#define MESSAGE_WRAPPER_HPP

#include "EmbeddedMessage.h"
#include <stdio.h>

// Wrapper C++ para compatibilidad con código existente
class MessageWrapper {
private:
    EmbeddedMessage_t* m_msg;
    bool m_owns_message;

public:
    // Constructor por defecto
    MessageWrapper() : m_msg(nullptr), m_owns_message(false) {}
    
    // Constructor que obtiene mensaje del pool
    MessageWrapper(uint8_t msgId, ModuleId_t sender, ModuleId_t receiver) 
        : m_msg(nullptr), m_owns_message(false) {
        allocate();
        if (m_msg) {
            EmbeddedMessage_Create(m_msg, msgId, sender, receiver);
        }
    }
    
    // Constructor con payload
    MessageWrapper(uint8_t msgId, ModuleId_t sender, ModuleId_t receiver, 
                  const uint8_t* data, uint8_t length) 
        : m_msg(nullptr), m_owns_message(false) {
        allocate();
        if (m_msg) {
            EmbeddedMessage_CreateWithPayload(m_msg, msgId, sender, receiver, data, length);
        }
    }
    
    // Constructor de copia
    MessageWrapper(const MessageWrapper& other) 
        : m_msg(nullptr), m_owns_message(false) {
        if (other.m_msg && other.isValid()) {
            allocate();
            if (m_msg) {
                EmbeddedMessage_Copy(m_msg, other.m_msg);
            }
        }
    }
    
    // Operador de asignación
    MessageWrapper& operator=(const MessageWrapper& other) {
        if (this != &other) {
            // Liberar mensaje actual si es propietario
            if (m_owns_message && m_msg) {
                MessagePool_Free(m_msg);
                m_msg = nullptr;
                m_owns_message = false;
            }
            
            // Copiar del otro wrapper
            if (other.m_msg && other.isValid()) {
                allocate();
                if (m_msg) {
                    EmbeddedMessage_Copy(m_msg, other.m_msg);
                }
            }
        }
        return *this;
    }
    
    // Destructor
    ~MessageWrapper() {
        if (m_owns_message && m_msg) {
            MessagePool_Free(m_msg);
        }
    }
    
    // ====== Getters compatibles con Message class original ======
    
    uint8_t getId() const { return m_msg ? m_msg->id : 0; }
    ModuleId_t getSender() const { return m_msg ? m_msg->sender : MODULE_DISPATCHER; }
    ModuleId_t getReceiver() const { return m_msg ? m_msg->receiver : MODULE_DISPATCHER; }
    uint8_t getLength() const { return m_msg ? m_msg->length : 0; }
    const uint8_t* getPayload() const { return m_msg ? m_msg->payload : nullptr; }
    uint32_t getTimestamp() const { return m_msg ? m_msg->timestamp : 0; }
    
    // ====== Setters ======
    
    void setId(uint8_t id) { if (m_msg) m_msg->id = id; }
    void setSender(ModuleId_t sender) { if (m_msg) m_msg->sender = sender; }
    void setReceiver(ModuleId_t receiver) { if (m_msg) m_msg->receiver = receiver; }
    
    MessageResult_t setPayload(const uint8_t* data, uint8_t length) {
        if (!m_msg) return MSG_RESULT_ERROR_MESSAGE_NULL;
        return EmbeddedMessage_SetPayload(m_msg, data, length);
    }
    
    // ====== Compatibilidad con código legacy ======
    
    // Acceso directo para migración gradual
    EmbeddedMessage_t* getRawMessage() { return m_msg; }
    const EmbeddedMessage_t* getRawMessage() const { return m_msg; }
    
    // Estado del wrapper
    bool isValid() const { return m_msg != nullptr; }
    bool ownsMessage() const { return m_owns_message; }
    
    // Para usar en colas que esperan punteros
    EmbeddedMessage_t* release() {
        EmbeddedMessage_t* temp = m_msg;
        m_msg = nullptr;
        m_owns_message = false;
        return temp;
    }
    
    // Para tomar ownership de un mensaje del pool
    void adopt(EmbeddedMessage_t* msg) {
        if (m_owns_message && m_msg) {
            MessagePool_Free(m_msg);
        }
        m_msg = msg;
        m_owns_message = (msg != nullptr);
    }
    
    // Factory methods para compatibilidad
    static MessageWrapper createMessage(uint8_t msgId, ModuleId_t sender, ModuleId_t receiver) {
        return MessageWrapper(msgId, sender, receiver);
    }
    
    static MessageWrapper createMessageWithPayload(uint8_t msgId, ModuleId_t sender, 
                                                 ModuleId_t receiver, const uint8_t* data, 
                                                 uint8_t length) {
        return MessageWrapper(msgId, sender, receiver, data, length);
    }

private:
    void allocate() {
        m_msg = MessagePool_Allocate();
        m_owns_message = (m_msg != nullptr);
    }
};

// ====== Funciones de utilidad para migración ======

// Convertir ModuleId_t del enum class original al nuevo enum
inline ModuleId_t ConvertModuleId(uint8_t legacy_module_id) {
    return static_cast<ModuleId_t>(legacy_module_id);
}

// Para debugging y estadísticas
inline void PrintMessagePoolStats() {
    uint32_t total, allocated, max_allocated;
    MessagePool_GetStats(&total, &allocated, &max_allocated);
    
    // Usar printf embedded-friendly
    printf("MessagePool Stats:\n");
    printf("  Total: %u\n", (unsigned int)total);
    printf("  Allocated: %u\n", (unsigned int)allocated);
    printf("  Max Allocated: %u\n", (unsigned int)max_allocated);
    printf("  Usage: %u%%\n", (unsigned int)((allocated * 100) / total));
}

#endif // MESSAGE_WRAPPER_HPP