
#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdint.h>
#include <vector>
typedef enum ModuleId_t;

typedef struct {
    uint8_t id;         // ID del mensaje (2 bytes)
    ModuleId_t sender;   // ID del emisor (1 byte)
    ModuleId_t receiver;   // ID del receptor (1 byte)
    std::vector<uint8_t> payload;  // Datos del mensaje (n bytes)
} Messages_t;

typedef enum {
    MODULE_SENSOR_ACQ = 1,
    MODULE_STIMULUS = 2,
    MODULE_GPS = 3,
    MODULE_LORA_TX = 4,
    MODULE_LORA_RX = 5,
    MODULE_FSM = 6,
    MODULE_DISTANCE = 7,
    MODULE_FENCE_UPDATE = 8,
    MODULE_DISPATCHER = 9,
} ModuleId_t

//  Mensajes de control general
#define MSG_ID_START             0x01  // Iniciar sistema o proceso
#define MSG_ID_STOP              0x02  // Detener sistema o proceso
#define MSG_ID_RESET             0x03  // Reiniciar hardware o módulo
#define MSG_ID_ACK               0x04  // Acknowledge  

// Estado y sincronización
#define MSG_ID_STATE_UPDATE      0x10  // Cambio de estado de FSM
#define MSG_ID_STATUS_REQUEST    0x11  // Pedido de estado a un módulo
#define MSG_ID_STATUS_RESPONSE   0x12  // Respuesta de estado
#define MSG_ID_TIME_SYNC         0x13  // Sincronización de tiempo

// GPS
#define MSG_ID_GPS_DATA          0x20  // Coordenadas GPS actualizadas
#define MSG_ID_GPS_TIMEOUT       0x21  // No hay señal GPS

// Cerca virtual
#define MSG_ID_FENCE_UPDATE      0x30  // Nueva cerca virtual enviada
#define MSG_ID_FENCE_STATUS      0x31  // Estado de la cerca actual
#define MSG_ID_FENCE_BREACH      0x32  // Animal se salió del perímetro
#define MSG_ID_DISTANCE_TO_FENCE 0x33  // Distancia actual al límite

// Estímulos individuales
#define MSG_ID_STIMULUS_VIBRATION_REQUEST   0x40  // Pedido de vibración
#define MSG_ID_STIMULUS_VIBRATION_FEEDBACK  0x41  // Resultado de vibración

#define MSG_ID_STIMULUS_SOUND_REQUEST       0x42  // Pedido de sonido
#define MSG_ID_STIMULUS_SOUND_FEEDBACK      0x43  // Resultado de sonido

#define MSG_ID_STIMULUS_ELECTRIC_REQUEST    0x44  // Pedido de estímulo eléctrico
#define MSG_ID_STIMULUS_ELECTRIC_FEEDBACK   0x45  // Resultado de estímulo eléctrico

// Sensores y diagnóstico
#define MSG_ID_SENSOR_DATA       0x50  // Datos de sensores (IMU, acelerómetro, etc.)

// Comunicación LoRa
#define MSG_ID_LORA_TX           0x60  // Pedido de transmisión
#define MSG_ID_LORA_RX           0x61  // Mensaje recibido por LoRa
#define MSG_ID_LORA_JOINED       0x62  // Join exitoso en red LoRa

// Errores y log
#define MSG_ID_ERROR             0xF0  // Error genérico
#define MSG_ID_DIAGNOSTIC        0xF1  // Mensaje de diagnóstico
#define MSG_ID_LOG               0xF2  // Mensaje de log
#define MSG_ID_WARNING           0xF3  // Advertencia no crítica
#define MSG_ID_INFO              0xF4  // Información general


#endif // MESSAGES_H