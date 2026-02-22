#include "FreeRTOS.h"
#include "ina226.h"
#include "lsm6dso.h"
#include "sam_m10q.h"
#include "task.h"
#include "sensorAcqTask.h"
#include "cmsis_os.h"
#include "EmbeddedMessage.h"
#include "gps_data.h"
#include "fsm_helper.h"  // Para BURST_SIZE
#define RTOS_PRINTF_AUTO  // Enable smart printf routing
#include "rtos_printf.h"
#include <string.h>

// Declaraciones externas de las colas
extern osMessageQueueId_t sensorAcqQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;

void sensorAcqTask(void *argument) {
    // CRITICAL: Use static objects to avoid stack overflow
    // These objects must have static lifetime and be initialized explicitly
    static SamM10q gps;
    static Lsm6dso imu;
    static Ina226 inaGps;
    static Ina226 inaImu;
    static Ina226 inaMcu;
    
    // CRITICAL: Static buffer for IMU burst - must persist after osMessageQueuePut
    // The dispatcher processes messages asynchronously, so payload must remain valid
    static AccRaw imuBurstBuffer[BURST_SIZE];
    
    // Explicit initialization - must be called once at task startup
    if (!gps.init(gpsAddress)) {
        RTOS_LOG_ERROR("[SENSOR_ACQ] Failed to initialize GPS\r\n");
        // Retry or handle error appropriately
    }
    
    if (!imu.init(imuAddress)) {
        RTOS_LOG_ERROR("[SENSOR_ACQ] Failed to initialize IMU\r\n");
        // Retry or handle error appropriately
    }
    
    if (!inaGps.init(configs[gpsIndex].address, configs[gpsIndex].rShunt, 
                     configs[gpsIndex].currentLSB, Ina226Averaging::AVG_64,
             Ina226BusConvTime::CT_1_1MS,
             Ina226ShuntConvTime::CT_1_1MS,
             Ina226Mode::SHUNT_BUS_CONTINUOUS)) {
        RTOS_LOG_ERROR("[SENSOR_ACQ] Failed to initialize INA226 GPS\r\n");
    }
    
    if (!inaImu.init(configs[imuIndex].address, configs[imuIndex].rShunt, 
                     configs[imuIndex].currentLSB, Ina226Averaging::AVG_64,
            Ina226BusConvTime::CT_1_1MS,
            Ina226ShuntConvTime::CT_1_1MS,
            Ina226Mode::SHUNT_BUS_CONTINUOUS)) {
        RTOS_LOG_ERROR("[SENSOR_ACQ] Failed to initialize INA226 IMU\r\n");
    }
    
    if (!inaMcu.init(configs[mcuIndex].address, configs[mcuIndex].rShunt, 
                     configs[mcuIndex].currentLSB, Ina226Averaging::AVG_128, 
                     Ina226BusConvTime::CT_1_1MS, Ina226ShuntConvTime::CT_1_1MS, 
                     Ina226Mode::SHUNT_BUS_CONTINUOUS)) {
        RTOS_LOG_ERROR("[SENSOR_ACQ] Failed to initialize INA226 MCU\r\n");
    }

    EmbeddedMessage_t *msgReceived = NULL;
    EmbeddedMessage_t *msgToSend = NULL;
    gpsRateSpeed rateGPS;
    RTOS_LOG_INFO("[SENSOR_ACQ] Task initialized successfully\r\n");
    gpsData_t gpsData = {0.0, 0.0, 0};
    float imuData[3] = {0.0, 0.0, 0.0};
    static uint32_t stackMonitorCounter = 0;
    static uint32_t iTow = 0; // Variable para monitorear iTOW y detectar reinicios del GPS
    while(1) {
        // Monitorear stack cada ~10 segundos
        if (++stackMonitorCounter >= 10) {
            UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
            RTOS_LOG_INFO("[SENSOR_ACQ] Stack libre: %u words (%u bytes)\r\n", 
                         stackLeft, stackLeft * 4);
            stackMonitorCounter = 0;
        }
        
        //// Leer todos los sensores periódicamente
            // gps.read_gps_position();
            // uint8_t fix = gps.flags & 0x01; // Bit 0 indica si hay fix
            // gpsData.latitude = gps.latitude;
            // gpsData.longitude = gps.longitude;
            // gpsData.fix = fix;
            // RTOS_LOG_DEBUG("[SENSOR_ACQ] GPS read: lat %.6f, lon %.6f, fix %d\r\n", gpsData.latitude, gpsData.longitude, gpsData.fix);

            // imu.readAcceleration();
            // imuData[0] = imu.ax;
            // imuData[1] = imu.ay;
            // imuData[2] = imu.az;
            // RTOS_LOG_DEBUG("[SENSOR_ACQ] IMU read: (%.6f g, %.6f g, %.6f g)\r\n", imu.ax/1000, imu.ay/1000, imu.az/1000);

            // inaGps.readCurrent_mA();
            // RTOS_LOG_DEBUG("[SENSOR_ACQ] INA GPS current read: %.3f mA\r\n", inaGps.current);
            // inaImu.readCurrent_mA();
            // RTOS_LOG_DEBUG("[SENSOR_ACQ] INA IMU current read: %.3f mA\r\n", inaImu.current);
            // inaMcu.readCurrent_mA();
            // RTOS_LOG_DEBUG("[SENSOR_ACQ] INA MCU current read: %.3f mA\r\n", inaMcu.current);
        // Verificar si hay mensajes de solicitud
        if (osMessageQueueGet(sensorAcqQueueHandle, &msgReceived, NULL, 0) == osOK) {
            RTOS_LOG_DEBUG("[SENSOR_ACQ] Received message ID:%d from module:%d\r\n", msgReceived->id, msgReceived->sender);
            
            switch (msgReceived->id) {
                case MSG_ID_REQUEST_GPS: {
                    // Leer GPS solo cuando se solicita
                    gps.read_gps_position();
                    gpsData.latitude = gps.latitude;
                    gpsData.longitude = gps.longitude;
                    gpsData.fix = gps.flags & 0x01; // Bit 0 indica si hay fix
                    RTOS_LOG_DEBUG("[SENSOR_ACQ] GPS read: lat %.6f, lon %.6f, fix %d\r\n", gpsData.latitude, gpsData.longitude, gpsData.fix);
                    if(gpsData.fix && gps.iTow != iTow) { // Solo enviar si hay fix y iTOW ha cambiado (nuevo dato) 
                        msgToSend = MessagePool_Allocate();
                        if (msgToSend != NULL) {
                            RTOS_LOG_DEBUG("[SENSOR_ACQ] alocado\r\n");
                            EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SEND_GPS, MODULE_SENSOR_ACQ, MODULE_FSM, (uint8_t*)&gpsData, sizeof(gpsData_t));
                            osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                            RTOS_LOG_DEBUG("[SENSOR_ACQ] Sent GPS data to FSM\r\n");
                            msgToSend = NULL;
                        }
                        else {
                            RTOS_LOG_DEBUG("[SENSOR_ACQ] se lleno la pile\r\n");
                        }
                        iTow = gps.iTow; // Actualizar iTOW
                    }
                    
                    break;
                }

                case MSG_ID_REQUEST_GPS_CONFIGURATION_PSM: {
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        gps.configure_gps(40, M10Q_NUM_DATA_ELEMENTS);
                        EmbeddedMessage_Create(msgToSend, MSG_ID_SEND_GPS_CONFIGURATION_PSM, MODULE_SENSOR_ACQ, MODULE_FSM);
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[SENSOR_ACQ] Sent GPS Confirmation PSMOO to FSM\r\n");
                        msgToSend = NULL;
                    }
                    break;
                }

                case MSG_ID_REQUEST_IMU: {
                    // Leer IMU solo cuando se solicita
                    imu.readAcceleration();
                    imuData[0] = imu.ax;
                    imuData[1] = imu.ay;
                    imuData[2] = imu.az;
                    RTOS_LOG_DEBUG("[SENSOR_ACQ] IMU read on request: (%.3f, %.3f, %.3f) mg\r\n", imu.ax, imu.ay, imu.az);
                    
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
                                RTOS_LOG_WARN("[SENSOR_ACQ] ⚠️  I2C error at sample %d (using previous value)\r\n", i);
                            } else {
                                imuBurstBuffer[i].ax = 0;
                                imuBurstBuffer[i].ay = 0;
                                imuBurstBuffer[i].az = 0;
                                RTOS_LOG_ERROR("[SENSOR_ACQ] ❌ I2C error at sample %d (no previous value, using zeros)\r\n", i);
                            }
                        }
                        
                        // Timing preciso: esperar hasta próxima muestra (26 Hz = 38.46 ms)
                        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(38));
                    }
                    
                    // Solo reportar errores I2C si hubo
                    if (errorCount > 0) {
                        RTOS_LOG_WARN("[SENSOR_ACQ] ⚠️  %d I2C errors during burst (%d/%d OK)\r\n", errorCount, successfulReads, BURST_SIZE);
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
                    
                    RTOS_LOG_INFO("[SENSOR_ACQ] 📊 Features: var=%lu, range_total=%u (z=%u), z_ratio=%u%% from %d samples\r\n", 
                                  features.var_total, range_total, range_z, z_ratio, BURST_SIZE);
                    
                    // Enviar solo features (12 bytes) en vez de burst completo (312 bytes)
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SEND_IMU_BURST, MODULE_SENSOR_ACQ, msgReceived->sender, (uint8_t*)&features, sizeof(BurstFeatures));
                        osStatus_t status = osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 100);
                        
                        if (status == osOK) {
                            // Success
                        } else {
                            RTOS_LOG_ERROR("[SENSOR_ACQ] ❌ Failed to send IMU features (osStatus: %d)\r\n", status);
                            MessagePool_Free(msgToSend);
                        }
                        msgToSend = NULL;
                    } else {
                        RTOS_LOG_ERROR("[SENSOR_ACQ] ❌ Failed to allocate message for IMU features (pool full?)\r\n");
                    }
                    break;
                }
        
                case MSG_ID_REQUEST_INA_MCU: {
                    // Leer INA MCU solo cuando se solicita
                    inaMcu.readCurrent_mA();
                    RTOS_LOG_DEBUG("[SENSOR_ACQ] INA MCU read on request: %.3f mA\r\n", inaMcu.current);
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SEND_INA_MCU, MODULE_SENSOR_ACQ, MODULE_FSM, (uint8_t*)&(inaMcu.current), sizeof(float));
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[SENSOR_ACQ] Sent INA MCU data to FSM\r\n");
                        msgToSend = NULL;
                    }
                    break;
                }
                
                case MSG_ID_REQUEST_INA_GPS: {
                    // Leer INA GPS solo cuando se solicita
                    inaGps.readCurrent_mA();
                    RTOS_LOG_DEBUG("[SENSOR_ACQ] INA GPS read on request: %.3f mA\r\n", inaGps.current);
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SEND_INA_GPS, MODULE_SENSOR_ACQ, MODULE_FSM, (uint8_t*)&(inaGps.current), sizeof(float));
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[SENSOR_ACQ] Sent INA GPS data to FSM\r\n");
                        msgToSend = NULL;
                    }
                    break;
                }

                case MSG_ID_REQUEST_INA_IMU: {
                    // Leer INA IMU solo cuando se solicita
                    inaImu.readCurrent_mA();
                    RTOS_LOG_DEBUG("[SENSOR_ACQ] INA IMU read on request: %.3f mA\r\n", inaImu.current);
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SEND_INA_IMU, MODULE_SENSOR_ACQ, MODULE_FSM, (uint8_t*)&(inaImu.current), sizeof(float));
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[SENSOR_ACQ] Sent INA IMU data to FSM\r\n");
                        msgToSend = NULL;
                    }
                    break;
                }

                case MSG_ID_GPS_REQUEST_CONFIG: {
                    rateGPS = static_cast<gpsRateSpeed>(msgReceived->payload[0]);
                    if (gps.set_new_acq_time(rateGPS)) {
                        RTOS_LOG_DEBUG("[SENSOR_ACQ] GPS acquisition time set to %d\r\n", static_cast<int>(rateGPS));
                    }
                    else {
                        RTOS_LOG_WARN("[SENSOR_ACQ] Failed to set GPS acquisition time\r\n");
                    }
                    break;
                }

                // Console UART requests - leer sensores on-demand
                case MSG_ID_CONSOLE_READ_GPS: {
                    gps.read_gps_position();
                    gpsData.latitude = gps.latitude;
                    gpsData.longitude = gps.longitude;
                    gpsData.fix = gps.flags & 0x01;
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SENSOR_GPS_DATA, 
                                                         MODULE_SENSOR_ACQ, MODULE_CONSOLE, 
                                                         (uint8_t*)&gpsData, sizeof(gpsData_t));
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[SENSOR_ACQ] Sent GPS data to console\r\n");
                        msgToSend = NULL;
                    }
                    break;
                }

                case MSG_ID_CONSOLE_READ_IMU: {
                    imu.readAcceleration();
                    imuData[0] = imu.ax;
                    imuData[1] = imu.ay;
                    imuData[2] = imu.az;
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SENSOR_IMU_DATA, 
                                                         MODULE_SENSOR_ACQ, MODULE_CONSOLE, 
                                                         (uint8_t*)imuData, 3 * sizeof(float));
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[SENSOR_ACQ] Sent IMU data to console\r\n");
                        msgToSend = NULL;
                    }
                    break;
                }

                case MSG_ID_CONSOLE_READ_INA_GPS: {
                    inaGps.readCurrent_mA();
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SENSOR_INA_GPS_DATA, 
                                                         MODULE_SENSOR_ACQ, MODULE_CONSOLE, 
                                                         (uint8_t*)&(inaGps.current), sizeof(float));
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[SENSOR_ACQ] Sent INA GPS data to console\r\n");
                        msgToSend = NULL;
                    }
                    break;
                }

                case MSG_ID_CONSOLE_READ_INA_IMU: {
                    inaImu.readCurrent_mA();
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SENSOR_INA_IMU_DATA, 
                                                         MODULE_SENSOR_ACQ, MODULE_CONSOLE, 
                                                         (uint8_t*)&(inaImu.current), sizeof(float));
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[SENSOR_ACQ] Sent INA IMU data to console\r\n");
                        msgToSend = NULL;
                    }
                    break;
                }

                case MSG_ID_CONSOLE_READ_INA_MCU: {
                    inaMcu.readCurrent_mA();
                    
                    msgToSend = MessagePool_Allocate();
                    if (msgToSend != NULL) {
                        EmbeddedMessage_CreateWithPayload(msgToSend, MSG_ID_SENSOR_INA_MCU_DATA, 
                                                         MODULE_SENSOR_ACQ, MODULE_CONSOLE, 
                                                         (uint8_t*)&(inaMcu.current), sizeof(float));
                        osMessageQueuePut(dispatcherQueueHandle, &msgToSend, 0, 0);
                        RTOS_LOG_DEBUG("[SENSOR_ACQ] Sent INA MCU data to console\r\n");
                        msgToSend = NULL;
                    }
                    break;
                }
        
                default:
                    RTOS_LOG_WARN("[SENSOR_ACQ] Unknown message ID: %d\r\n", msgReceived->id);
                    break;
            }
      
            MessagePool_Free(msgReceived);
            msgReceived = NULL;
        } // Fin del if (osMessageQueueGet)

        osDelay(500);
    } // Fin del while(1)
} // Fin de sensorAcqTask