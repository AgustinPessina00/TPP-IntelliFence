#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "EmbeddedMessage.h"
#include "Test/test_data.h"
#define RTOS_PRINTF_AUTO
#include "rtos_printf.h"
#include <string.h>

// ============================================================================
// SENSOR ACQUISITION MOCK FOR FSM TESTING
// ============================================================================
// Este módulo reemplaza MODULE_SENSOR_ACQ durante las pruebas
// Responde a las solicitudes de la FSM con datos de prueba estáticos

extern osMessageQueueId_t fsmQueueHandle;
extern osMessageQueueId_t sensorAcqQueueHandle;

// Variable para habilitar/deshabilitar el modo test
static bool testModeEnabled = true;

// ============================================================================
// SENSOR ACQ TASK - VERSION TEST
// ============================================================================

void sensorAcqTask_Test(void *argument) {
    EmbeddedMessage_t msgReceived;
    EmbeddedMessage_t msgResponse;
    
    rtos_printf("[TEST_SENSOR_ACQ] Iniciado en modo TEST\n");
    rtos_printf("[TEST_SENSOR_ACQ] GPS samples: %d, IMU samples: %d\n", 
                TEST_GPS_DATA_COUNT, TEST_IMU_DATA_COUNT);
    
    // Inicializar datos de prueba
    TestData_Init();
    
    for(;;) {
        // Esperar mensajes de la FSM
        if (osMessageQueueGet(sensorAcqQueueHandle, &msgReceived, NULL, osWaitForever) == osOK) {
            
            // Limpiar estructura de respuesta
            memset(&msgResponse, 0, sizeof(EmbeddedMessage_t));
            msgResponse.dest = MODULE_FSM;
            msgResponse.src = MODULE_SENSOR_ACQ;
            
            switch(msgReceived.msgId) {
                // ============================================================
                // REQUEST POSITION (GPS)
                // ============================================================
                case MSG_REQUEST_POSITION: {
                    const TestGPSData_t* gpsData = TestData_GetNextGPS();
                    
                    if (gpsData != nullptr) {
                        msgResponse.msgId = MSG_RESPONSE_POSITION;
                        
                        // Copiar latitud y longitud en el payload
                        memcpy(&msgResponse.payload[0], &gpsData->position.latitude, sizeof(double));
                        memcpy(&msgResponse.payload[8], &gpsData->position.longitude, sizeof(double));
                        msgResponse.payloadSize = 16;
                        
                        rtos_printf("[TEST_GPS %02d] Lat: %.6f, Lon: %.6f | %s -> %s\n",
                                    TestData_GetGPSIndex() - 1,
                                    gpsData->position.latitude,
                                    gpsData->position.longitude,
                                    gpsData->description,
                                    (gpsData->expectedZone == GREEN_ZONE) ? "GREEN" :
                                    (gpsData->expectedZone == LIGHT_BLUE_ZONE) ? "LIGHT_BLUE" :
                                    (gpsData->expectedZone == BLUE_ZONE) ? "BLUE" :
                                    (gpsData->expectedZone == DARK_BLUE_ZONE) ? "DARK_BLUE" :
                                    (gpsData->expectedZone == YELLOW_ZONE) ? "YELLOW" :
                                    (gpsData->expectedZone == RED_ZONE) ? "RED" : "BLACK");
                    } else {
                        // No hay más datos
                        msgResponse.msgId = MSG_ERROR_TIMEOUT;
                        rtos_printf("[TEST_GPS] WARNING: No hay mas datos de prueba\n");
                    }
                    
                    // Enviar respuesta a FSM
                    osMessageQueuePut(fsmQueueHandle, &msgResponse, 0, 0);
                    break;
                }
                
                // ============================================================
                // REQUEST ACCELEROMETER (IMU)
                // ============================================================
                case MSG_REQUEST_ACCELEROMETER: {
                    const TestIMUData_t* imuData = TestData_GetNextIMU();
                    
                    if (imuData != nullptr) {
                        msgResponse.msgId = MSG_RESPONSE_ACCELEROMETER;
                        
                        // Copiar aceleración (ax, ay, az) en el payload
                        memcpy(&msgResponse.payload[0], &imuData->acceleration.ax, sizeof(double));
                        memcpy(&msgResponse.payload[8], &imuData->acceleration.ay, sizeof(double));
                        memcpy(&msgResponse.payload[16], &imuData->acceleration.az, sizeof(double));
                        msgResponse.payloadSize = 24;
                        
                        rtos_printf("[TEST_IMU %02d] ax: %.2f, ay: %.2f, az: %.2f | %s -> %s\n",
                                    TestData_GetIMUIndex() - 1,
                                    imuData->acceleration.ax,
                                    imuData->acceleration.ay,
                                    imuData->acceleration.az,
                                    imuData->description,
                                    (imuData->expectedState == CowState::SLEEP) ? "SLEEP" :
                                    (imuData->expectedState == CowState::GRAZING) ? "GRAZING" : "MOVEMENT");
                    } else {
                        // No hay más datos
                        msgResponse.msgId = MSG_ERROR_TIMEOUT;
                        rtos_printf("[TEST_IMU] WARNING: No hay mas datos de prueba\n");
                    }
                    
                    // Enviar respuesta a FSM
                    osMessageQueuePut(fsmQueueHandle, &msgResponse, 0, 0);
                    break;
                }
                
                // ============================================================
                // REQUEST ZONE
                // ============================================================
                case MSG_REQUEST_ZONE: {
                    // Extraer posición del payload (enviada por FSM)
                    Position cowPos;
                    memcpy(&cowPos.latitude, &msgReceived.payload[0], sizeof(double));
                    memcpy(&cowPos.longitude, &msgReceived.payload[8], sizeof(double));
                    
                    // Obtener zona basada en la posición
                    TestZoneData_t zoneData = TestData_GetZone(cowPos);
                    
                    msgResponse.msgId = MSG_RESPONSE_ZONE;
                    msgResponse.payload[0] = static_cast<uint8_t>(zoneData.zone);
                    memcpy(&msgResponse.payload[1], &zoneData.distance, sizeof(double));
                    msgResponse.payloadSize = 9;
                    
                    rtos_printf("[TEST_ZONE] Zona: %s, Distancia: %.1fm\n",
                                (zoneData.zone == GREEN_ZONE) ? "GREEN" :
                                (zoneData.zone == LIGHT_BLUE_ZONE) ? "LIGHT_BLUE" :
                                (zoneData.zone == BLUE_ZONE) ? "BLUE" :
                                (zoneData.zone == DARK_BLUE_ZONE) ? "DARK_BLUE" :
                                (zoneData.zone == YELLOW_ZONE) ? "YELLOW" :
                                (zoneData.zone == RED_ZONE) ? "RED" : "BLACK",
                                zoneData.distance);
                    
                    // Enviar respuesta a FSM
                    osMessageQueuePut(fsmQueueHandle, &msgResponse, 0, 0);
                    break;
                }
                
                // ============================================================
                // SET GPS RATE
                // ============================================================
                case MSG_SET_GPS_RATE: {
                    uint32_t rate = msgReceived.payload[0];
                    rtos_printf("[TEST_GPS_RATE] Nueva tasa: %d Hz (ignorado en test)\n", rate);
                    
                    // En modo test, simplemente confirmar
                    msgResponse.msgId = MSG_RESPONSE_OK;
                    osMessageQueuePut(fsmQueueHandle, &msgResponse, 0, 0);
                    break;
                }
                
                // ============================================================
                // COMANDOS NO IMPLEMENTADOS EN TEST
                // ============================================================
                default:
                    rtos_printf("[TEST_SENSOR_ACQ] WARNING: Comando no implementado: 0x%02X\n", 
                                msgReceived.msgId);
                    break;
            }
        }
        
        // Delay para simular tiempo de adquisición
        osDelay(50);
    }
}

// ============================================================================
// FUNCIONES DE CONTROL DEL MODO TEST
// ============================================================================

void TestMode_Enable(void) {
    testModeEnabled = true;
    rtos_printf("[TEST] Modo test HABILITADO\n");
}

void TestMode_Disable(void) {
    testModeEnabled = false;
    rtos_printf("[TEST] Modo test DESHABILITADO\n");
}

bool TestMode_IsEnabled(void) {
    return testModeEnabled;
}

void TestMode_Reset(void) {
    TestData_Reset();
    rtos_printf("[TEST] Datos de prueba RESETEADOS\n");
}
