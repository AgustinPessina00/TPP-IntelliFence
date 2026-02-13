/**
 * @file BURST_IMU_INTEGRATION_EXAMPLE.cpp
 * @brief Ejemplo de cómo integrar el sistema de burst en sensorAcqTask.cpp
 * 
 * ESTE ARCHIVO ES SOLO REFERENCIA - NO COMPILAR DIRECTAMENTE
 * 
 * Copiar/adaptar el código relevante a sensorAcqTask.cpp
 */

#include "lsm6dso.h"
#include "messages_id.h"
#include "EmbeddedMessage.h"
#include "fsm_helper.h"  // Para BURST_SIZE

// ============================================================================
// OPCIÓN 1: Burst on-demand (respuesta a MSG_ID_REQUEST_IMU)
// ============================================================================

void sensorAcqTask(void *argument) {
    extern Lsm6dso imu;
    EmbeddedMessage_t *msg = nullptr;
    TickType_t xLastWakeTime;
    
    for(;;) {
        // Esperar mensaje de solicitud
        if (osMessageQueueGet(sensorAcqQueueHandle, &msg, nullptr, 500) == osOK) {
            switch (msg->id) {
                case MSG_ID_REQUEST_IMU: {
                    // Colectar burst de N muestras
                    AccRaw samples[BURST_SIZE];
                    xLastWakeTime = xTaskGetTickCount();
                    
                    for (uint16_t i = 0; i < BURST_SIZE; i++) {
                        // Leer aceleración raw
                        if (imu.readAcceleration() == I2C_OK) {
                            samples[i].ax = imu.axRaw;
                            samples[i].ay = imu.ayRaw;
                            samples[i].az = imu.azRaw;
                        } else {
                            // Error en lectura - llenar con último valor válido
                            if (i > 0) {
                                samples[i] = samples[i-1];
                            } else {
                                samples[i] = {0, 0, 0};
                            }
                        }
                        
                        // Esperar hasta próxima muestra (26 Hz = 38.46 ms)
                        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(38));
                    }
                    
                    // Enviar burst a FSM
                    EmbeddedMessage_t *replyMsg = MessagePool_Allocate();
                    if (replyMsg != nullptr) {
                        EmbeddedMessage_CreateWithPayload(
                            replyMsg,
                            MSG_ID_SEND_IMU_BURST,
                            MODULE_SENSOR_ACQ,
                            msg->sender,  // Responder al que pidió
                            (uint8_t*)samples,
                            sizeof(AccRaw) * BURST_SIZE
                        );
                        osMessageQueuePut(dispatcherQueueHandle, &replyMsg, 0, 100);
                        RTOS_LOG_DEBUG("[SENSOR_ACQ] IMU burst sent (%d samples)\r\n", BURST_SIZE);
                    }
                    
                    MessagePool_Free(msg);
                    break;
                }
                
                // ... otros casos ...
            }
        }
    }
}


// ============================================================================
// OPCIÓN 2: Burst periódico (envío automático cada X segundos)
// ============================================================================

void sensorAcqTask_AutoBurst(void *argument) {
    extern Lsm6dso imu;
    AccRaw samples[BURST_SIZE];
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    // Configuración: enviar burst cada 10 segundos
    const TickType_t BURST_INTERVAL = pdMS_TO_TICKS(10000);
    TickType_t nextBurstTime = xLastWakeTime + BURST_INTERVAL;
    
    for(;;) {
        TickType_t now = xTaskGetTickCount();
        
        // ¿Es momento de colectar burst?
        if (now >= nextBurstTime) {
            // Colectar BURST_SIZE muestras
            for (uint16_t i = 0; i < BURST_SIZE; i++) {
                if (imu.readAcceleration() == I2C_OK) {
                    samples[i].ax = imu.axRaw;
                    samples[i].ay = imu.ayRaw;
                    samples[i].az = imu.azRaw;
                }
                
                vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(38));  // 26 Hz
            }
            
            // Enviar burst automáticamente a FSM
            EmbeddedMessage_t *msg = MessagePool_Allocate();
            if (msg != nullptr) {
                EmbeddedMessage_CreateWithPayload(
                    msg,
                    MSG_ID_SEND_IMU_BURST,
                    MODULE_SENSOR_ACQ,
                    MODULE_FSM,
                    (uint8_t*)samples,
                    sizeof(AccRaw) * BURST_SIZE
                );
                osMessageQueuePut(dispatcherQueueHandle, &msg, 0, 100);
            }
            
            // Programar próximo burst
            nextBurstTime = now + BURST_INTERVAL;
        }
        
        // Procesar otros mensajes mientras esperamos
        EmbeddedMessage_t *msg = nullptr;
        if (osMessageQueueGet(sensorAcqQueueHandle, &msg, nullptr, 100) == osOK) {
            // Procesar mensaje...
            MessagePool_Free(msg);
        }
    }
}


// ============================================================================
// OPCIÓN 3: Burst adaptativo (frecuencia según estado de la vaca)
// ============================================================================

typedef enum {
    BURST_MODE_SLOW,    // Cada 30s (vaca quieta/durmiendo)
    BURST_MODE_MEDIUM,  // Cada 10s (vaca pastando)
    BURST_MODE_FAST     // Cada 5s  (vaca en movimiento/cerca de límite)
} BurstMode_t;

void sensorAcqTask_Adaptive(void *argument) {
    extern Lsm6dso imu;
    AccRaw samples[BURST_SIZE];
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    BurstMode_t currentMode = BURST_MODE_MEDIUM;
    TickType_t nextBurstTime = xLastWakeTime;
    
    const TickType_t intervals[] = {
        pdMS_TO_TICKS(30000),  // SLOW
        pdMS_TO_TICKS(10000),  // MEDIUM
        pdMS_TO_TICKS(5000)    // FAST
    };
    
    for(;;) {
        TickType_t now = xTaskGetTickCount();
        
        // Procesar mensajes de cambio de modo
        EmbeddedMessage_t *msg = nullptr;
        if (osMessageQueueGet(sensorAcqQueueHandle, &msg, nullptr, 0) == osOK) {
            if (msg->id == MSG_ID_IMU_RATE_CHANGE) {
                // Cambiar frecuencia de burst según estado
                currentMode = (BurstMode_t)msg->payload[0];
                RTOS_LOG_DEBUG("[SENSOR_ACQ] Burst mode changed: %d\r\n", currentMode);
            }
            MessagePool_Free(msg);
        }
        
        // ¿Es momento de burst?
        if (now >= nextBurstTime) {
            // Colectar burst
            for (uint16_t i = 0; i < BURST_SIZE; i++) {
                if (imu.readAcceleration() == I2C_OK) {
                    samples[i].ax = imu.axRaw;
                    samples[i].ay = imu.ayRaw;
                    samples[i].az = imu.azRaw;
                }
                vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(38));
            }
            
            // Enviar burst
            EmbeddedMessage_t *burstMsg = MessagePool_Allocate();
            if (burstMsg != nullptr) {
                EmbeddedMessage_CreateWithPayload(
                    burstMsg,
                    MSG_ID_SEND_IMU_BURST,
                    MODULE_SENSOR_ACQ,
                    MODULE_FSM,
                    (uint8_t*)samples,
                    sizeof(AccRaw) * BURST_SIZE
                );
                osMessageQueuePut(dispatcherQueueHandle, &burstMsg, 0, 100);
            }
            
            // Programar próximo según modo actual
            nextBurstTime = now + intervals[currentMode];
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


// ============================================================================
// NOTAS DE IMPLEMENTACIÓN
// ============================================================================

/**
 * CONSIDERACIONES:
 * 
 * 1. **Frecuencia de muestreo**: 26 Hz (38.46 ms entre muestras)
 *    - Resolución suficiente para detectar movimientos de vaca
 *    - Balance entre resolución y consumo
 * 
 * 2. **Duración del burst**: 2 segundos (52 muestras)
 *    - Estadísticamente significativo
 *    - No demasiado largo (bloqueo aceptable)
 * 
 * 3. **Manejo de errores I2C**:
 *    - Si falla una lectura, repetir último valor válido
 *    - Nunca enviar burst con valores 0 (falso positivo de quietud)
 * 
 * 4. **Uso de memoria**:
 *    - AccRaw samples[52] = 52 * 6 bytes = 312 bytes en stack
 *    - Alternativa: buffer estático si stack es limitado
 * 
 * 5. **Timing preciso**:
 *    - Usar vTaskDelayUntil() para timing consistente
 *    - NO usar vTaskDelay() (acumula drift)
 * 
 * 6. **Compatibilidad backward**:
 *    - FSM soporta tanto MSG_ID_SEND_IMU_BURST (nuevo)
 *    - Como MSG_ID_SEND_IMU (legacy single-sample)
 *    - Permite transición gradual
 */
