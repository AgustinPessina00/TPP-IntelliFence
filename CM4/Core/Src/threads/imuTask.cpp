#include "FreeRTOS.h"
#include "lsm6dso.h"
#include "task.h"
#include "imuTask.h"
#include "cmsis_os.h"
#include "EmbeddedMessage.h"
#include "fsm_helper.h"  // Para BURST_SIZE
#define RTOS_PRINTF_AUTO  // Enable smart printf routing
#include "rtos_printf.h"
#include <string.h>

// Declaraciones externas de las colas
extern osMessageQueueId_t imuQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;

void imuTask(void *argument) {
    // CRITICAL: Use static objects to avoid stack overflow
    static Lsm6dso imu;
    
    // CRITICAL: Static buffer for IMU burst - must persist after osMessageQueuePut
    // The dispatcher processes messages asynchronously, so payload must remain valid
    static AccRaw imuBurstBuffer[BURST_SIZE];
    
    // Explicit initialization - must be called once at task startup
    if (!imu.init(imuAddress)) {
        RTOS_LOG_ERROR("[IMU_TASK] Failed to initialize IMU\r\n");
        // Retry or handle error appropriately
    }

    EmbeddedMessage_t *msgReceived = NULL;
    EmbeddedMessage_t *msgToSend = NULL;
    float imuData[3] = {0.0, 0.0, 0.0};
    
    RTOS_LOG_INFO("[IMU_TASK] Task initialized successfully\r\n");
    
    while(1) {

        // Bloquear hasta que llegue un mensaje (modo eficiente)
        if (osMessageQueueGet(imuQueueHandle, &msgReceived, NULL, osWaitForever) == osOK) {
            RTOS_LOG_DEBUG("[IMU_TASK] Received message ID:%d from module:%d\r\n", msgReceived->id, msgReceived->sender);
            
            switch (msgReceived->id) {
                case MSG_ID_REQUEST_IMU: {
                    RTOS_LOG_INFO("[IMU_TASK] MSG_ID_REQUEST_IMU received from module %d\r\n", msgReceived->sender);
                    RTOS_LOG_DEBUG("[IMU_TASK] Starting IMU burst collection (%d samples @ 26Hz)\r\n", BURST_SIZE);
                    
                    // Use static buffer (defined at function scope) - CRITICAL for async message processing
                    TickType_t xLastWakeTime = xTaskGetTickCount();
                    uint16_t successfulReads = 0;
                    uint16_t errorCount = 0;
                    
                    // Colectar BURST_SIZE muestras con timing preciso
                    for (uint16_t i = 0; i < BURST_SIZE; i++) {
                        // Leer aceleración raw del LSM6DSO
                        if (imu.readAcceleration() == I2C_OK) {
                            imuBurstBuffer[i].ax = imu.axRaw;
                            imuBurstBuffer[i].ay = imu.ayRaw;
                            imuBurstBuffer[i].az = imu.azRaw;
                            successfulReads++;
                        } else {
                            errorCount++;
                            // Error I2C: repetir último valor válido (evita ceros falsos)
                            if (i > 0) {
                                imuBurstBuffer[i] = imuBurstBuffer[i-1];
                                RTOS_LOG_WARN("[IMU_TASK]  I2C error at sample %d (using previous value)\r\n", i);
                            } else {
                                imuBurstBuffer[i].ax = 0;
                                imuBurstBuffer[i].ay = 0;
                                imuBurstBuffer[i].az = 0;
                                RTOS_LOG_ERROR("[IMU_TASK] I2C error at sample %d (no previous value, using zeros)\r\n", i);
                            }
                        }
                        
                        // Timing preciso: esperar hasta próxima muestra (26 Hz = 38.46 ms)
                        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(38));
                    }
                    
                    // Solo reportar errores I2C si hubo
                    if (errorCount > 0) {
                        RTOS_LOG_WARN("[IMU_TASK]  %d I2C errors during burst (%d/%d OK)\r\n", errorCount, successfulReads, BURST_SIZE);
                    }
                    
                    // Compute VARIANCE + RANGE + Z_RATIO features (12 bytes payload)
                    
                    // Paso 1: Calcular promedios y buscar min/max (un solo loop)
                    int64_t sum_ax = 0, sum_ay = 0, sum_az = 0;
                    int16_t min_ax = INT16_MAX, max_ax = INT16_MIN;
                    int16_t min_ay = INT16_MAX, max_ay = INT16_MIN;
                    int16_t min_az = INT16_MAX, max_az = INT16_MIN;
                    
                    for (uint16_t i = 0; i < BURST_SIZE; i++) {
                        int16_t ax = imuBurstBuffer[i].ax;
                        int16_t ay = imuBurstBuffer[i].ay;
                        int16_t az = imuBurstBuffer[i].az;
                        
                        sum_ax += ax;
                        sum_ay += ay;
                        sum_az += az;
                        
                        if (ax < min_ax) min_ax = ax;
                        if (ax > max_ax) max_ax = ax;
                        if (ay < min_ay) min_ay = ay;
                        if (ay > max_ay) max_ay = ay;
                        if (az < min_az) min_az = az;
                        if (az > max_az) max_az = az;
                    }
                    
                    int32_t avg_ax = (int32_t)(sum_ax / BURST_SIZE);
                    int32_t avg_ay = (int32_t)(sum_ay / BURST_SIZE);
                    int32_t avg_az = (int32_t)(sum_az / BURST_SIZE);
                    
                    // Paso 2: Calcular varianzas - Var(X) = E[(X - μ)²]
                    // Usamos esta fórmula para evitar problemas numéricos con offset de gravedad
                    int64_t sum_var_x = 0, sum_var_y = 0, sum_var_z = 0;
                    
                    for (uint16_t i = 0; i < BURST_SIZE; i++) {
                        int32_t dx = imuBurstBuffer[i].ax - avg_ax;
                        int32_t dy = imuBurstBuffer[i].ay - avg_ay;
                        int32_t dz = imuBurstBuffer[i].az - avg_az;
                        
                        sum_var_x += (int64_t)dx * dx;
                        sum_var_y += (int64_t)dy * dy;
                        sum_var_z += (int64_t)dz * dz;
                    }
                    
                    int32_t var_x = (int32_t)(sum_var_x / BURST_SIZE);
                    int32_t var_y = (int32_t)(sum_var_y / BURST_SIZE);
                    int32_t var_z = (int32_t)(sum_var_z / BURST_SIZE);
                    
                    // Paso 3: Calcular rangos (max - min) para capturar amplitud de movimiento
                    uint16_t range_x = (uint16_t)(max_ax - min_ax);
                    uint16_t range_y = (uint16_t)(max_ay - min_ay);
                    uint16_t range_z = (uint16_t)(max_az - min_az);
                    uint16_t range_total = range_x + range_y + range_z;
                    
                    // Paso 4: Calcular dominancia de Z (para detectar GRAZING = movimiento vertical)
                    uint16_t z_ratio = (range_total > 0) ? ((range_z * 100) / range_total) : 0;
                    
                    // Paso 5: Crear estructura de features
                    BurstFeatures features;
                    features.var_total = (uint32_t)(var_x + var_y + var_z);
                    features.range_z = range_z;
                    features.range_total = range_total;
                    features.z_ratio = z_ratio;
                    features.reserved = 0;
                    
                    RTOS_LOG_INFO("[IMU_TASK] 📊 Features: var=%lu, range_total=%u (z=%u), z_ratio=%u%% from %d samples\r\n", 
                                  features.var_total, range_total, range_z, z_ratio, BURST_SIZE);
                    
                    // Enviar solo features (12 bytes) en vez de burst completo (312 bytes)
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SEND_IMU_BURST, MODULE_IMU, msgReceived->sender, (uint8_t*)&features, sizeof(BurstFeatures));
                        osStatus_t status = osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 100);
                        
                        if (status == osOK) {
                            // Success
                        } else {
                            RTOS_LOG_ERROR("[IMU_TASK] Failed to send IMU features (osStatus: %d)\r\n", status);
                            MessagePool_Free(msgToSend);
                        }
                        msgToSend = NULL;
                    } else {
                        RTOS_LOG_ERROR("[IMU_TASK] Failed to allocate message for IMU features (pool full?)\r\n");
                    }
                    break;
                }

                case MSG_ID_CONSOLE_READ_IMU:
                    imu.readAcceleration();
                    imuData[0] = imu.ax;
                    imuData[1] = imu.ay;
                    imuData[2] = imu.az;
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SENSOR_IMU_DATA, 
                                                         MODULE_IMU, MODULE_CONSOLE, 
                                                         (uint8_t*)imuData, 3 * sizeof(float));
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[IMU_TASK] Sent IMU data to console\r\n");
                        msgToSend = NULL;
                    }
                    break;
        
                default:
                    RTOS_LOG_WARN("[IMU_TASK] Unknown message ID: %d\r\n", msgReceived->id);
                    break;
            }
      
            MessagePool_Free(msgReceived);
            msgReceived = NULL;
        }
    }
}
