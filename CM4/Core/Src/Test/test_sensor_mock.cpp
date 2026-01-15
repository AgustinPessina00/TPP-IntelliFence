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
extern osMessageQueueId_t dispatcherQueueHandle;

// Variable para habilitar/deshabilitar el modo test
static bool testModeEnabled = true;

// ============================================================================
// SENSOR ACQ TASK - VERSION TEST
// ============================================================================

extern "C" void sensorAcqTask_Test(void *argument) {
    EmbeddedMessage_t *msgReceived = NULL;
    EmbeddedMessage_t *msgResponse = NULL;
    
    rtos_printf("[TEST_SENSOR_ACQ] Iniciado en modo TEST\n");
    rtos_printf("[TEST_SENSOR_ACQ] GPS samples: %d, IMU samples: %d\n", 
                TEST_GPS_DATA_COUNT, TEST_IMU_DATA_COUNT);
    
    // Inicializar datos de prueba
    TestData_Init();
    
    for(;;) {
        // Esperar mensajes de la FSM (recibir PUNTERO al mensaje)
        if (osMessageQueueGet(sensorAcqQueueHandle, &msgReceived, NULL, osWaitForever) == osOK) {
            
            // Allocar mensaje de respuesta
            msgResponse = MessagePool_Allocate();
            if (msgResponse == NULL) {
                rtos_printf("[TEST_SENSOR_ACQ] ERROR: No se pudo allocar mensaje de respuesta\n");
                MessagePool_Free(msgReceived);
                continue;
            }
            
            // Configurar respuesta
            msgResponse->sender = MODULE_SENSOR_ACQ;
            msgResponse->receiver = MODULE_FSM;
            
            switch(msgReceived->id) {
                // ============================================================
                // REQUEST GPS
                // ============================================================
                case MSG_ID_REQUEST_GPS: {
                    const TestGPSData_t* gpsData = TestData_GetNextGPS();
                    
                    if (gpsData != nullptr) {
                        msgResponse->id = MSG_ID_SEND_GPS;
                        
                        // Copiar latitud y longitud en el payload
                        memcpy(&msgResponse->payload[0], &gpsData->position.latitude, sizeof(float));
                        memcpy(&msgResponse->payload[4], &gpsData->position.longitude, sizeof(float));
                        msgResponse->length = 8;
                        
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
                        msgResponse->id = MSG_ID_ERROR;
                        rtos_printf("[TEST_GPS] WARNING: No hay mas datos de prueba\n");
                    }
                    
                    // Enviar respuesta a FSM vía DISPATCHER
                    osMessageQueuePut(dispatcherQueueHandle, &msgResponse, 0, 0);
                    break;
                }
                
                // ============================================================
                // REQUEST IMU
                // ============================================================
                case MSG_ID_REQUEST_IMU: {
                    const TestIMUData_t* imuData = TestData_GetNextIMU();
                    
                    if (imuData != nullptr) {
                        msgResponse->id = MSG_ID_SEND_IMU;
                        
                        // Copiar aceleración (ax, ay, az) en el payload
                        memcpy(&msgResponse->payload[0], &imuData->acceleration.ax, sizeof(double));
                        memcpy(&msgResponse->payload[8], &imuData->acceleration.ay, sizeof(double));
                        memcpy(&msgResponse->payload[16], &imuData->acceleration.az, sizeof(double));
                        msgResponse->length = 24;
                        
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
                        msgResponse->id = MSG_ID_ERROR;
                        rtos_printf("[TEST_IMU] WARNING: No hay mas datos de prueba\n");
                    }
                    
                    // Enviar respuesta a FSM vía DISPATCHER
                    osMessageQueuePut(dispatcherQueueHandle, &msgResponse, 0, 0);
                    break;
                }
                
                // ============================================================
                // REQUEST ZONE AND DISTANCE
                // ============================================================
                case MSG_ID_REQUEST_ZONE_AND_DISTANCE_TO_FENCE: {
                    // Extraer posición del payload (enviada por FSM)
                    Position cowPos;
                    memcpy(&cowPos.latitude, &msgReceived->payload[0], sizeof(float));
                    memcpy(&cowPos.longitude, &msgReceived->payload[4], sizeof(float));
                    
                    // Obtener zona basada en la posición
                    TestZoneData_t zoneData = TestData_GetZone(cowPos);
                    
                    msgResponse->id = MSG_ID_SEND_ZONE_AND_DISTANCE_TO_FENCE;
                    msgResponse->payload[0] = static_cast<uint8_t>(zoneData.zone);
                    memcpy(&msgResponse->payload[1], &zoneData.distance, sizeof(float));
                    msgResponse->length = 5;
                    
                    rtos_printf("[TEST_ZONE] Zona: %s, Distancia: %.1fm\n",
                                (zoneData.zone == GREEN_ZONE) ? "GREEN" :
                                (zoneData.zone == LIGHT_BLUE_ZONE) ? "LIGHT_BLUE" :
                                (zoneData.zone == BLUE_ZONE) ? "BLUE" :
                                (zoneData.zone == DARK_BLUE_ZONE) ? "DARK_BLUE" :
                                (zoneData.zone == YELLOW_ZONE) ? "YELLOW" :
                                (zoneData.zone == RED_ZONE) ? "RED" : "BLACK",
                                zoneData.distance);
                    
                    // Enviar respuesta a FSM vía DISPATCHER
                    osMessageQueuePut(dispatcherQueueHandle, &msgResponse, 0, 0);
                    break;
                }
                
                // ============================================================
                // GPS CONFIG REQUEST
                // ============================================================
                case MSG_ID_GPS_REQUEST_CONFIG: {
                    uint32_t rate = msgReceived->payload[0];
                    rtos_printf("[TEST_GPS_RATE] Nueva tasa: %d Hz (ignorado en test)\n", rate);
                    
                    // En modo test, simplemente confirmar
                    msgResponse->id = MSG_ID_GPS_CONFIG_RESPONSE;
                    msgResponse->length = 0;
                    osMessageQueuePut(dispatcherQueueHandle, &msgResponse, 0, 0);
                    break;
                }
                
                // ============================================================
                // COMANDOS NO IMPLEMENTADOS EN TEST
                // ============================================================
                default:
                    rtos_printf("[TEST_SENSOR_ACQ] WARNING: Comando no implementado: 0x%02X\n", 
                                msgReceived->id);
                    MessagePool_Free(msgResponse);
                    msgResponse = NULL;
                    break;
            }
            
            // Liberar mensaje recibido
            MessagePool_Free(msgReceived);
        }
        
        // Delay para simular tiempo de adquisición
        osDelay(50);
    }
}

// ============================================================================
// FUNCIONES DE CONTROL DEL MODO TEST
// ============================================================================

extern "C" void TestMode_Enable(void) {
    testModeEnabled = true;
    rtos_printf("[TEST] Modo test HABILITADO\n");
}

extern "C" void TestMode_Disable(void) {
    testModeEnabled = false;
    rtos_printf("[TEST] Modo test DESHABILITADO\n");
}

extern "C" bool TestMode_IsEnabled(void) {
    return testModeEnabled;
}

extern "C" void TestMode_Reset(void) {
    TestData_Reset();
    rtos_printf("[TEST] Datos de prueba RESETEADOS\n");
}
