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

// ============================================================================
// BURST CLASSIFICATION SYSTEM
// ============================================================================

// Helper: absolute value for uint32_t from int32_t
static inline uint32_t uabs32(int32_t x) {
    return (x < 0) ? (uint32_t)(-x) : (uint32_t)x;
}

// Compute burst features from N raw samples
static BurstFeatures computeBurstFeatures(const AccRaw* samples, uint16_t N, 
                                         uint32_t g2, uint32_t th_peak) {
    uint64_t sum_d = 0;
    uint64_t sum_a2 = 0;
    uint16_t peaks = 0;

    for (uint16_t i = 0; i < N; i++) {
        int32_t ax = samples[i].ax;
        int32_t ay = samples[i].ay;
        int32_t az = samples[i].az;

        // a² = ax² + ay² + az²
        uint32_t a2 = (uint32_t)(ax*ax + ay*ay + az*az);
        sum_a2 += a2;

        // d = |a² - g²|
        uint32_t d = uabs32((int32_t)a2 - (int32_t)g2);
        sum_d += d;

        if (d > th_peak) peaks++;
    }

    BurstFeatures features;
    features.E = (uint32_t)(sum_d / N);
    features.peaks = peaks;
    
    return features;
}

// Classify cow state from burst features
static CowState classifyFromFeatures(uint32_t E, uint16_t peaks,
                                    uint32_t th_rest,
                                    uint32_t th_move_strong,
                                    uint16_t th_peaks_graze) {
    // Quieta (rest state - not necessarily sleeping yet)
    if (E < th_rest) {
        return CowState::SLEEP;  // Note: actual sleep determined by FSM time tracking
    }

    // Active: grazing has high peak count but lower overall energy
    if ((peaks >= th_peaks_graze) && (E < th_move_strong)) {
        return CowState::GRAZING;
    }

    return CowState::MOVEMENT;
}

// State tracking for persistence and anti-flapping
static struct {
    CowState history[5];     // Last 5 classifications
    uint8_t historyIndex;    // Circular buffer index
    uint8_t historyCount;    // How many valid entries
    CowState lastCommitted;  // Last state sent to Cow
    uint32_t quietTicks;     // Consecutive ticks in quiet state
    uint32_t g2;             // Calibrated g² value (auto-updated)
    bool g2Initialized;      // g² calibration flag
} stateTracker = {
    .history = {CowState::MOVEMENT, CowState::MOVEMENT, CowState::MOVEMENT, 
                CowState::MOVEMENT, CowState::MOVEMENT},
    .historyIndex = 0,
    .historyCount = 0,
    .lastCommitted = CowState::MOVEMENT,
    .quietTicks = 0,
    .g2 = 268000000,  // Initial estimate for ±2g (~16384² LSB)
    .g2Initialized = false
};

// Configuration parameters
#define QUIET_MINUTES_TO_SLEEP  5      // Minutes of quiet before declaring SLEEP
#define TICKS_PER_MINUTE        30     // Assuming ~2s burst period
#define SLEEP_THRESHOLD_TICKS   (QUIET_MINUTES_TO_SLEEP * TICKS_PER_MINUTE)

// Update g² calibration (IIR filter when quiet)
static void updateG2Calibration(const AccRaw* samples, uint16_t N, uint32_t E) {
    // Only calibrate when quiet (low E)
    uint32_t th_quiet = stateTracker.g2 / 100;  // 1% of g²
    
    if (E < th_quiet) {
        // Calculate average a²
        uint64_t sum_a2 = 0;
        for (uint16_t i = 0; i < N; i++) {
            int32_t ax = samples[i].ax;
            int32_t ay = samples[i].ay;
            int32_t az = samples[i].az;
            sum_a2 += (uint32_t)(ax*ax + ay*ay + az*az);
        }
        uint32_t avg_a2 = (uint32_t)(sum_a2 / N);
        
        // IIR: g2 = 0.95*g2 + 0.05*avg_a2 → (g2*19 + avg_a2)/20
        stateTracker.g2 = (stateTracker.g2 * 19 + avg_a2) / 20;
        stateTracker.g2Initialized = true;
        
        RTOS_LOG_DEBUG("[FSM] g² calibrated: %lu\r\n", stateTracker.g2);
    }
}

// Process burst and update cow state with persistence
void updateStateFromBurst(Cow& cow, const AccRaw* samples, uint16_t N) {
    // Calculate dynamic thresholds based on current g²
    uint32_t th_peak        = stateTracker.g2 / 50;   // ~2%
    uint32_t th_rest        = stateTracker.g2 / 200;  // ~0.5%
    uint32_t th_move_strong = stateTracker.g2 / 20;   // ~5%
    uint16_t th_peaks_graze = N / 3;                  // ~33% of samples
    
    // Compute features
    BurstFeatures features = computeBurstFeatures(samples, N, stateTracker.g2, th_peak);
    
    RTOS_LOG_DEBUG("[FSM] Burst: E=%lu, peaks=%u, g²=%lu\r\n", 
                  features.E, features.peaks, stateTracker.g2);
    
    // Update g² calibration
    updateG2Calibration(samples, N, features.E);
    
    // Classify
    CowState candidate = classifyFromFeatures(features.E, features.peaks,
                                             th_rest, th_move_strong, th_peaks_graze);
    
    // Add to history
    stateTracker.history[stateTracker.historyIndex] = candidate;
    stateTracker.historyIndex = (stateTracker.historyIndex + 1) % 5;
    if (stateTracker.historyCount < 5) stateTracker.historyCount++;
    
    // Persistence check: need 2 consecutive matching states to commit
    bool shouldCommit = false;
    if (stateTracker.historyCount >= 2) {
        uint8_t prev = (stateTracker.historyIndex + 5 - 2) % 5;
        uint8_t curr = (stateTracker.historyIndex + 5 - 1) % 5;
        
        if (stateTracker.history[prev] == stateTracker.history[curr]) {
            shouldCommit = true;
        }
    }
    
    if (shouldCommit) {
        CowState newState = candidate;
        
        // Handle quiet → sleep transition (requires X minutes)
        if (candidate == CowState::SLEEP) {
            stateTracker.quietTicks++;
            
            if (stateTracker.quietTicks >= SLEEP_THRESHOLD_TICKS) {
                newState = CowState::SLEEP;  // Real sleep after X minutes
                RTOS_LOG_INFO("[FSM] Entering SLEEP after %lu ticks\r\n", stateTracker.quietTicks);
            } else {
                // Still quiet, but not yet sleeping - keep previous active state
                newState = stateTracker.lastCommitted;
            }
        } else {
            // Any activity resets quiet counter and exits sleep
            stateTracker.quietTicks = 0;
        }
        
        // Only update if changed
        if (newState != stateTracker.lastCommitted) {
            cow.updateState(newState);
            stateTracker.lastCommitted = newState;
            RTOS_LOG_INFO("[FSM] State committed: %d\r\n", (int)newState);
        }
    }
}

// Legacy single-sample version (DEPRECATED - use updateStateFromBurst instead)
void updateState(Cow& cow) {
    RTOS_LOG_WARN("[FSM] updateState(single sample) is deprecated - use updateStateFromBurst\r\n");
    // Fallback: assume some default state
    cow.updateState(CowState::MOVEMENT);
}
