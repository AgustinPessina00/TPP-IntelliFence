/**
 * @file fsm_helper.cpp
 * @brief Implementation of helper functions shared across all FSM implementations
 */

#include "fsm_helper.h"
#include "rtos_printf.h"
#include "getZone.h"
#include "gps_data.h"
#include <cstring>
#include <cmath>

// ============================================================================
// MESSAGE QUEUE OPERATIONS
// ============================================================================

void sendMessage(uint8_t msgId, ModuleId_t dest) {
    EmbeddedMessage_t *msg = MessagePool_Allocate();
    if (msg != nullptr) {
        EmbeddedMessage_Create(msg, msgId, MODULE_FSM, dest);
        osMessageQueuePut(dispatcherQueueHandle, &msg, 0, 100);
    } else {
        RTOS_LOG_ERROR("[FSM] Failed to allocate message for ID:%d\r\n", msgId);
    }
}

HAL_StatusTypeDef dequeueMessage(EmbeddedMessage_t **msg) {
    if (osMessageQueueGet(fsmQueueHandle, msg, nullptr, 0) == osOK) {
        return HAL_OK;
    }
    return HAL_ERROR;
}

HAL_StatusTypeDef waitForMessage(uint8_t expectedMsgId, TimeoutContext_t& timeout, 
                                 EmbeddedMessage_t** msg, bool newMessage) {
    // Si no hay mensaje nuevo, seguimos esperando
    if (!newMessage || msg == nullptr || *msg == nullptr) {
        return HAL_BUSY;
    }
    
    // Si es el mensaje esperado, retornar OK
    if ((*msg)->id == expectedMsgId) {
        return HAL_OK;
    }
    
    // Mensaje inesperado - log
    RTOS_LOG_WARN("[FSM] Unexpected message ID:%d (expected:%d)\r\n", (*msg)->id, expectedMsgId);
    return HAL_BUSY;
}

// ============================================================================
// MESSAGE PROCESSORS
// ============================================================================

HAL_StatusTypeDef processGpsMessage(EmbeddedMessage_t *msg, Cow& cow, bool& validPosition) {
    if (msg->length == sizeof(gpsData_t)) {
        gpsData_t gpsData;
        memcpy(&gpsData, msg->payload, sizeof(gpsData_t));
        
        RTOS_LOG_DEBUG("[FSM] GPS position: lat=%.6f, lon=%.6f, fix=%d\r\n", 
                      gpsData.latitude, gpsData.longitude, gpsData.fix);
        
        // Validar fix GPS
        if (gpsData.fix) {
            cow.updatePosition({gpsData.latitude, gpsData.longitude});
            validPosition = true;
            RTOS_LOG_DEBUG("[FSM] Valid GPS fix - position updated\r\n");
        } else {
            validPosition = false;
            RTOS_LOG_WARN("[FSM] No GPS fix - position not updated\r\n");
        }
        
        return HAL_OK;
    }
    
    RTOS_LOG_WARN("[FSM] Invalid GPS payload size (expected %d, got %d)\r\n", 
                 sizeof(gpsData_t), msg->length);
    validPosition = false;
    return HAL_ERROR;
}

HAL_StatusTypeDef processLoRaTxResponse(EmbeddedMessage_t *msg) {
    RTOS_LOG_DEBUG("[FSM] LoRa TX confirmed position send\r\n");
    return HAL_OK;
}

HAL_StatusTypeDef processFenceMessage(EmbeddedMessage_t *msg, Fence& fence) {
    // Variables estáticas para reconstruir fence fragmentado
    static Vertex receivedVertices[MAX_VERTICES];
    static uint8_t totalExpectedFragments = 0;
    static uint8_t receivedFragments = 0;
    static uint8_t totalVerticesReceived = 0;
    
    // Leer header del fragmento
    uint8_t fragmentNum = msg->payload[0];
    uint8_t totalFragments = msg->payload[1];
    uint8_t verticesInFragment = msg->payload[2];
    
    RTOS_LOG_DEBUG("[FSM] Received fence fragment %d/%d (%d vertices)\r\n",
                  fragmentNum + 1, totalFragments, verticesInFragment);
    
    // Primer fragmento: inicializar
    if (fragmentNum == 0) {
        totalExpectedFragments = totalFragments;
        receivedFragments = 0;
        totalVerticesReceived = 0;
    }
    
    // Copiar vértices de este fragmento
    uint8_t payloadOffset = 3;  // Después del header
    const uint8_t VERTEX_SIZE = 2 * sizeof(float);  // float lat + float lon
    
    for (uint8_t i = 0; i < verticesInFragment; i++) {
        if (totalVerticesReceived < MAX_VERTICES) {
            memcpy(&receivedVertices[totalVerticesReceived],
                   &msg->payload[payloadOffset],
                   VERTEX_SIZE);
            totalVerticesReceived++;
            payloadOffset += VERTEX_SIZE;
        }
    }
    
    receivedFragments++;
    
    // ¿Recibimos todos los fragmentos?
    if (receivedFragments == totalExpectedFragments) {
        RTOS_LOG_INFO("[FSM] All fence fragments received (%d vertices total)\r\n",
                     totalVerticesReceived);
        
        // Crear límites directamente desde buffer sin guardar vértices
        fence.createLimits(receivedVertices, totalVerticesReceived);
        
        // Reset para próxima recepción
        receivedFragments = 0;
        totalVerticesReceived = 0;
        
        return HAL_OK;
    } else {
        RTOS_LOG_DEBUG("[FSM] Waiting for more fragments (%d/%d)\r\n",
                      receivedFragments, totalExpectedFragments);
        return HAL_BUSY;  // Aún esperando más fragmentos
    }
}

HAL_StatusTypeDef processImuMessage(EmbeddedMessage_t *msg, Cow& cow) {
    if (msg->length == 3 * sizeof(double)) {
        double ax, ay, az;
        memcpy(&ax, msg->payload, sizeof(double));
        memcpy(&ay, msg->payload + sizeof(double), sizeof(double));
        memcpy(&az, msg->payload + 2 * sizeof(double), sizeof(double));
        
        RTOS_LOG_DEBUG("[FSM] IMU: ax=%.2f, ay=%.2f, az=%.2f g\r\n", ax, ay, az);
        
        cow.updateAcceleration({ax, ay, az});
        updateState(cow);
        
        return HAL_OK;
    }
    
    return HAL_ERROR;
}

HAL_StatusTypeDef processGpsConfigResponse(EmbeddedMessage_t *msg) {
    RTOS_LOG_DEBUG("[FSM] GPS config confirmed\r\n");
    return HAL_OK;
}

HAL_StatusTypeDef processStimulusResponse(EmbeddedMessage_t *msg) {
    RTOS_LOG_DEBUG("[FSM] Stimulus feedback received\r\n");
    return HAL_OK;
}

// ============================================================================
// HIGH-LEVEL OPERATIONS
// ============================================================================

void sendPosition(uint8_t msgId, ModuleId_t dest, Cow& cow) {
    EmbeddedMessage_t *msg = MessagePool_Allocate();
    if (msg != nullptr) {
        Position pos = cow.getPosition();
        float latitude = pos.latitude, longitude = pos.longitude;
        
        uint8_t data[2 * sizeof(float)];
        memcpy(data, &latitude, sizeof(float));
        memcpy(data + sizeof(float), &longitude, sizeof(float));
        
        EmbeddedMessage_CreateWithPayload(msg, msgId, MODULE_FSM, dest, data, 2 * sizeof(float));
        osMessageQueuePut(dispatcherQueueHandle, &msg, 0, 100);
    }
}

void sendZoneToStimulus(zone_t zone, ModuleId_t dest) {
    EmbeddedMessage_t *msg = MessagePool_Allocate();
    if (msg != nullptr) {
        EmbeddedMessage_CreateWithPayload(msg, MSG_ID_ZONE_CHANGE, MODULE_FSM, dest,
                                         (uint8_t*)&zone, sizeof(zone_t));
        osMessageQueuePut(dispatcherQueueHandle, &msg, 0, 100);
        RTOS_LOG_DEBUG("[FSM] Sent zone %d to STIMULUS\r\n", zone);
    }
}

void updateGpsAdqTime(GpsRate gpsRate) {
    EmbeddedMessage_t *msg = MessagePool_Allocate();
    if (msg != nullptr) {
        EmbeddedMessage_CreateWithPayload(msg, MSG_ID_GPS_REQUEST_CONFIG, MODULE_FSM, 
                                         MODULE_SENSOR_ACQ, (uint8_t*)&gpsRate, sizeof(GpsRate));
        osMessageQueuePut(dispatcherQueueHandle, &msg, 0, 100);
        RTOS_LOG_DEBUG("[FSM] GPS rate updated: %d\r\n", (int)gpsRate);
    }
}

void enterLowPowerSleep() {
    // __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    // HAL_SuspendTick();
    // HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    // HAL_ResumeTick();
    RTOS_LOG_DEBUG("[FSM] Woke from sleep\r\n");
}

// ============================================================================
// COW OPERATIONS
// ============================================================================

HAL_StatusTypeDef isInGreenZone(Cow& cow) {
    return (cow.getCurrentZone() == GREEN_ZONE) ? HAL_OK : HAL_ERROR;
}

static CowState classifyMotion(Acceleration acc) {
    double abs_ax = fabs(acc.ax);
    double abs_ay = fabs(acc.ay);
    double abs_az = fabs(acc.az);
    
    if (abs_ax < 0.05f && abs_ay < 0.05f && abs_az < 0.05f)
        return CowState::SLEEP;
    else if (abs_ax < 0.05f && abs_ay < 0.05f && abs_az > 0.1f)
        return CowState::GRAZING;
    else
        return CowState::MOVEMENT;
}

void updateState(Cow& cow) {
    cow.updateState(classifyMotion(cow.getAcceleration()));
}
