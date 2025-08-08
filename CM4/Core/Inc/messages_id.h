#ifndef MESSAGES_ID__H
#define MESSAGES_ID__H
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

// Sensor Acquisition
#define MSG_ID_SEND_GPS          0x20
#define MSG_ID_SEND_IMU          0x21
#define MSG_ID_SEND_INA_MCU      0x22
#define MSG_ID_SEND_INA_GPS      0x23
#define MSG_ID_SEND_INA_IMU      0x24
#define MSG_ID_REQUEST_GPS       0x25
#define MSG_ID_REQUEST_IMU       0x26
#define MSG_ID_REQUEST_INA_MCU   0x27
#define MSG_ID_REQUEST_INA_GPS   0x28
#define MSG_ID_REQUEST_INA_IMU   0x29

// Cerca virtual
#define MSG_ID_FENCE_UPDATE                         0x30    // Nueva cerca virtual enviada
#define MSG_ID_FENCE_STATUS                         0x31    // Estado de la cerca actual
#define MSG_ID_FENCE_BREACH                         0x32    // Animal se salió del perímetro
#define MSG_ID_REQUEST_DISTANCE_TO_FENCE            0x33    // Request Distancia actual al límite
#define MSG_ID_REQUEST_ZONE_TO_FENCE                0x34    // Request Zona actual
#define MSG_ID_REQUEST_ZONE_AND_DISTANCE_TO_FENCE   0x35    // Request Zona y Distancia actual al límite
#define MSG_ID_SEND_DISTANCE_TO_FENCE               0x36    // Distancia actual al límite
#define MSG_ID_SEND_ZONE_TO_FENCE                   0x37    // Zona actual
#define MSG_ID_SEND_ZONE_AND_DISTANCE_TO_FENCE      0x38    // Zona actual y Distancia actual al límite

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
#define MSG_ID_LORA_TX              0x60  // Pedido de transmisión
#define MSG_ID_LORA_RX              0x61  // Mensaje recibido por LoRa
#define MSG_ID_LORA_JOINED          0x62  // Join exitoso en red LoRa
#define MSG_ID_LORA_POSITION        0x63  // Transmisión de Posición
#define MSG_ID_LORA_VERTEXES        0x64  // Recepción de Vértices

// Configuración GPS
#define MSG_ID_GPS_REQUEST_CONFIG   0x70 // Pedido para configurar el rate del ADQTIME de GPS
#define MSG_ID_GPS_CONFIG_RESPONSE  0x71 // Respuesta del GPS

// Errores y log
#define MSG_ID_ERROR             0xF0  // Error genérico
#define MSG_ID_DIAGNOSTIC        0xF1  // Mensaje de diagnóstico
#define MSG_ID_LOG               0xF2  // Mensaje de log
#define MSG_ID_WARNING           0xF3  // Advertencia no crítica
#define MSG_ID_INFO              0xF4  // Información general

#endif