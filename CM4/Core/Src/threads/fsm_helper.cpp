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
    //HAL_GPIO_WritePin(GPS_EXTINT_GPIO_Port, GPS_EXTINT_Pin, GPIO_PIN_RESET);
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
        HAL_GPIO_WritePin(GPS_EXTINT_GPIO_Port, GPS_EXTINT_Pin, GPIO_PIN_SET);
        HAL_Delay(50);
        HAL_GPIO_WritePin(GPS_EXTINT_GPIO_Port, GPS_EXTINT_Pin, GPIO_PIN_RESET);
        EmbeddedMessage_CreateWithPayload(msg, MSG_ID_GPS_REQUEST_CONFIG, MODULE_FSM, 
                                         MODULE_GPS, (uint8_t*)&gpsRate, sizeof(GpsRate));
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
// State tracking for persistence and anti-flapping
static struct {
    CowState history[5];     // Last 5 classifications
    uint8_t historyIndex;    // Circular buffer index
    uint8_t historyCount;    // How many valid entries
    CowState lastCommitted;  // Last state sent to Cow
    uint32_t quietTicks;     // Consecutive ticks in quiet state
} stateTracker = {
    .history = {CowState::MOVEMENT, CowState::MOVEMENT, CowState::MOVEMENT, 
                CowState::MOVEMENT, CowState::MOVEMENT},
    .historyIndex = 0,
    .historyCount = 0,
    .lastCommitted = CowState::MOVEMENT,
    .quietTicks = 0
};

// Configuration parameters
#define QUIET_MINUTES_TO_SLEEP  5      // Minutes of quiet before declaring SLEEP
#define TICKS_PER_MINUTE        30     // Assuming ~2s burst period
#define SLEEP_THRESHOLD_TICKS   (QUIET_MINUTES_TO_SLEEP * TICKS_PER_MINUTE)

// Update cow state based on VARIANCE + RANGE + Z_RATIO (robust multi-feature approach)
void updateStateFromFeatures(Cow& cow, uint32_t var_total, uint16_t range_z, uint16_t range_total, uint16_t z_ratio) {
    // === MULTI-FEATURE THRESHOLDS ===
    // Basados en física del acelerómetro y comportamiento bovino real:
    // - Varianza captura "velocidad" del movimiento (alta = sacudidas/golpecitos)
    // - Rango captura "amplitud" del movimiento (alto = bajar/subir cabeza aunque sea lento)
    // - Z_ratio captura "verticalidad" (alto = principalmente movimiento vertical = pastando)
    
    // === THRESHOLDS CALIBRADOS EMPÍRICAMENTE (2026-02-15) ===
    // Basados en análisis estadístico de 23 muestras reales:
    // - QUIET: var=219-284 (mean=247), range=112-139 (mean=126) → 100% accuracy
    // - MOVEMENT: var=96k-95M, range=2.2k-50k → 100% accuracy
    // - Margen de seguridad: 3.5× sobre valores máximos observados
    
    // QUIET STRICT: Condiciones ideales (sin vibraciones externas)
    const uint32_t TH_VAR_QUIET_STRICT = 1000;     // Was 10k (QUIET max=284 × 3.5 ≈ 1000)
    const uint16_t TH_RANGE_QUIET_STRICT = 500;    // Was 1k (QUIET max=139 × 3.5 ≈ 500)
    
    // QUIET LOOSE (anti-vibración): Tolerancia para vibraciones ambientales (mesa, apoyos)
    // Basado en logs reales: var=1200-5400, range=280-750 en "mesa con vibración"
    const uint32_t TH_VAR_QUIET_LOOSE = 5000;      // Tolera vibraciones de mesa (var~1200-5400)
    const uint16_t TH_RANGE_QUIET_LOOSE = 900;     // Tolera amplitud de vibraciones (range~300-750)
    const uint16_t TH_RANGE_Z_ANTI_GRAZING = 450;  // Guard: range_z bajo descarta grazing falso
    const uint16_t TH_Z_RATIO_ANTI_GRAZING = 55;   // Guard: z_ratio bajo descarta grazing falso
    
    // MOVEMENT: Histéresis para evitar entradas falsas desde QUIET
    const uint32_t TH_VAR_MOVE_ENTER = 8000;       // Para ENTRAR a MOVEMENT desde QUIET (más estricto)
    const uint16_t TH_RANGE_MOVE_ENTER = 1200;     // Para ENTRAR a MOVEMENT desde QUIET (más estricto)
    
    const uint32_t TH_VAR_MOVE = 20000;            // Unchanged (min movement=96k, safety margin)
    const uint16_t TH_RANGE_Z = 500;               // Was 1.5k (min grazing z=271-857, margin=2×)
    const uint16_t TH_Z_RATIO = 60;                // Was 50% (grazing observado: 60-80%)
    
    const uint8_t QUIET_TO_SLEEP_COUNT = 10; // Repeticiones de QUIET para confirmar SLEEP
    
    RTOS_LOG_INFO("[FSM] 📊 Features: var=%lu, range=%u (z=%u), z_ratio=%u%% | TH: var_quiet_s<%lu, var_quiet_l<%lu, range_quiet_s<%u, range_quiet_l<%u, var_move_enter<%lu\r\n", 
                  var_total, range_total, range_z, z_ratio, 
                  TH_VAR_QUIET_STRICT, TH_VAR_QUIET_LOOSE, 
                  TH_RANGE_QUIET_STRICT, TH_RANGE_QUIET_LOOSE, 
                  TH_VAR_MOVE_ENTER);
    
    // Get current state for hysteresis logic
    CowState currentState = cow.getState();
    
    // === CLASIFICACIÓN MULTI-FEATURE CON ANTI-VIBRACIÓN E HISTÉRESIS ===
    CowState candidate;
    
    // QUIET STRICT: varianza baja Y rango bajo (condiciones ideales)
    if (var_total < TH_VAR_QUIET_STRICT && range_total < TH_RANGE_QUIET_STRICT) {
        candidate = CowState::QUIET;
    }
    // QUIET LOOSE (anti-vibración): tolera vibraciones de mesa/ambiente
    // - var y range moderados
    // - NO puede ser grazing (range_z bajo O z_ratio bajo)
    else if (var_total < TH_VAR_QUIET_LOOSE && range_total < TH_RANGE_QUIET_LOOSE &&
             (range_z < TH_RANGE_Z_ANTI_GRAZING || z_ratio < TH_Z_RATIO_ANTI_GRAZING)) {
        candidate = CowState::QUIET;
    }
    // GRAZING: rango Z alto + dominancia vertical + varianza no muy alta (movimiento lento vertical)
    else if (range_z > TH_RANGE_Z && z_ratio > TH_Z_RATIO && var_total < TH_VAR_MOVE) {
        candidate = CowState::GRAZING;
    }
    // MOVEMENT con HISTÉRESIS: más difícil entrar desde QUIET que desde otros estados
    else {
        // Si estaba en QUIET o SLEEP, requiere umbrales más altos para entrar a MOVEMENT
        if (currentState == CowState::QUIET || currentState == CowState::SLEEP) {
            if (var_total > TH_VAR_MOVE_ENTER || range_total > TH_RANGE_MOVE_ENTER) {
                candidate = CowState::MOVEMENT;
            } else {
                // No alcanza umbral estricto → mantener QUIET (absorbe vibraciones pequeñas)
                candidate = CowState::QUIET;
            }
        } else {
            // Desde otros estados (GRAZING, MOVEMENT), umbral normal (catch-all)
            candidate = CowState::MOVEMENT;
        }
    }
    
    // Log classification result (orden DEBE coincidir con enum CowState: SLEEP=0, QUIET=1, GRAZING=2, MOVEMENT=3)
    const char* stateNames[] = {"SLEEP", "QUIET", "GRAZING", "MOVEMENT"};
    const char* stateEmojis[] = {"😴", "🤫", "🐄", "🚶"};
    RTOS_LOG_INFO("[FSM] %s Classified as: %s (state %d)\r\n", 
                  stateEmojis[(int)candidate], stateNames[(int)candidate], (int)candidate);
    
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
    
    // Contador para transición QUIET → SLEEP
    static uint8_t quietConsecutiveCount = 0;
    
    // Commit state change if persistent
    if (shouldCommit) {
        CowState newState = stateTracker.history[(stateTracker.historyIndex + 5 - 1) % 5];
        CowState oldState = cow.getState();
        
        // Contador de QUIET consecutivos
        if (newState == CowState::QUIET) {
            quietConsecutiveCount++;
            RTOS_LOG_DEBUG("[FSM] QUIET count: %d/%d\r\n", quietConsecutiveCount, QUIET_TO_SLEEP_COUNT);
            
            // Después de N repeticiones de QUIET → cambiar a SLEEP
            if (quietConsecutiveCount >= QUIET_TO_SLEEP_COUNT) {
                RTOS_LOG_INFO("[FSM] 😴 QUIET repeated %d times → transitioning to SLEEP\r\n", quietConsecutiveCount);
                newState = CowState::SLEEP;
                quietConsecutiveCount = 0; // Reset counter
            }
        } else {
            // Cualquier otro estado resetea el contador
            if (quietConsecutiveCount > 0) {
                RTOS_LOG_DEBUG("[FSM] QUIET interrupted at count %d\r\n", quietConsecutiveCount);
            }
            quietConsecutiveCount = 0;
        }
        
        RTOS_LOG_DEBUG("[FSM] 🔄 Persistence check OK - Candidate committed\r\n");
        
        // Update cow state
        if (newState != oldState) {
            const char* stateNames[] = {"SLEEP", "QUIET", "GRAZING", "MOVEMENT"};
            const char* stateEmojis[] = {"😴", "🤫", "🐄", "🚶"};
            cow.updateState(newState);
            RTOS_LOG_INFO("[FSM] ════════════════════════════════════════\r\n");
            RTOS_LOG_INFO("[FSM] ✨ STATE CHANGE: %s %s → %s %s\r\n", 
                         stateEmojis[(int)oldState], stateNames[(int)oldState],
                         stateEmojis[(int)newState], stateNames[(int)newState]);
            RTOS_LOG_INFO("[FSM] ════════════════════════════════════════\r\n");
        }
    } else {
        RTOS_LOG_DEBUG("[FSM] ⏸️  Waiting for persistence (need 2 consecutive matches)\r\n");
    }
}

// Legacy single-sample version (DEPRECATED - use updateStateFromBurst instead)
void updateState(Cow& cow) {
    RTOS_LOG_WARN("[FSM] updateState(single sample) is deprecated - use updateStateFromBurst\r\n");
    // Fallback: assume some default state
    cow.updateState(CowState::MOVEMENT);
}
