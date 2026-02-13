/**
 * @file fsm_helper.h
 * @brief Helper functions shared across all FSM implementations
 * 
 * This file contains common operations used by multiple FSM modules:
 * - Message queue operations (send, dequeue, wait)
 * - Message processors for each message type
 * - High-level operations (send position, send zone, update GPS rate)
 * - Cow state operations
 */

#ifndef FSM_HELPER_H
#define FSM_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "cmsis_os.h"
#include "EmbeddedMessage.h"
#include "fsmTask.h"

#ifdef __cplusplus
}
#endif

#include "cow.h"
#include "fence.h"
#include "lsm6dso.h"  // For AccRaw

// Burst classification configuration
#define BURST_SIZE 52  // 2 seconds @ 26 Hz

// External queue handles
extern osMessageQueueId_t fsmQueueHandle;
extern osMessageQueueId_t dispatcherQueueHandle;
extern bool receivedMsgLoraRX;

// ============================================================================
// MESSAGE QUEUE OPERATIONS
// ============================================================================

/**
 * @brief Send a message to a destination module
 * @param msgId Message ID to send
 * @param dest Destination module ID
 */
void sendMessage(uint8_t msgId, ModuleId_t dest);

/**
 * @brief Dequeue next message from FSM queue (non-blocking)
 * @param msg Pointer to store dequeued message
 * @return HAL_OK if message dequeued, HAL_ERROR if queue empty
 */
HAL_StatusTypeDef dequeueMessage(EmbeddedMessage_t **msg);

/**
 * @brief Wait for a specific message type with timeout
 * @param expectedMsgId Expected message ID
 * @param timeout Timeout context
 * @param msg Pointer to received message (from FSM task dequeue)
 * @param newMessage Flag indicating if there's a new message
 * @return HAL_OK if expected message received, HAL_BUSY if still waiting, HAL_ERROR on timeout
 */
HAL_StatusTypeDef waitForMessage(uint8_t expectedMsgId, TimeoutContext_t& timeout, 
                                 EmbeddedMessage_t** msg, bool newMessage);

// ============================================================================
// MESSAGE PROCESSORS
// ============================================================================

/**
 * @brief Process GPS position message and update cow position
 * @param msg GPS message with lat/lon/fix payload (gpsData_t structure)
 * @param cow Cow object to update
 * @param validPosition Output parameter - true if GPS has valid fix, false otherwise
 * @return HAL_OK if processed successfully
 */
HAL_StatusTypeDef processGpsMessage(EmbeddedMessage_t *msg, Cow& cow, bool& validPosition);

/**
 * @brief Process LoRa TX confirmation message
 * @param msg LoRa TX feedback message
 * @return HAL_OK if processed successfully
 */
HAL_StatusTypeDef processLoRaTxResponse(EmbeddedMessage_t *msg);

/**
 * @brief Process fence vertices message (may be fragmented)
 * @param msg Fence message with vertices payload
 * @param fence Fence object to update
 * @return HAL_OK if all fragments received, HAL_BUSY if waiting for more
 */
HAL_StatusTypeDef processFenceMessage(EmbeddedMessage_t *msg, Fence& fence);

/**
 * @brief Process IMU acceleration message and update cow acceleration
 * @param msg IMU message with ax/ay/az payload
 * @param cow Cow object to update
 * @return HAL_OK if processed successfully
 */
HAL_StatusTypeDef processImuMessage(EmbeddedMessage_t *msg, Cow& cow);

/**
 * @brief Process GPS configuration response
 * @param msg GPS config confirmation
 * @return HAL_OK if processed successfully
 */
HAL_StatusTypeDef processGpsConfigResponse(EmbeddedMessage_t *msg);

/**
 * @brief Process stimulus module feedback
 * @param msg Stimulus feedback message
 * @return HAL_OK if processed successfully
 */
HAL_StatusTypeDef processStimulusResponse(EmbeddedMessage_t *msg);

// ============================================================================
// HIGH-LEVEL OPERATIONS
// ============================================================================

/**
 * @brief Send cow position to destination module
 * @param msgId Message ID to use
 * @param dest Destination module
 * @param cow Cow with position to send
 */
void sendPosition(uint8_t msgId, ModuleId_t dest, Cow& cow);

/**
 * @brief Send zone change to stimulus module
 * @param zone New zone value
 * @param dest Destination module (typically MODULE_STIMULUS)
 */
void sendZoneToStimulus(zone_t zone, ModuleId_t dest);

/**
 * @brief Update GPS acquisition rate
 * @param gpsRate New GPS rate (SLOW, MEDIUM, FAST, STOP)
 */
void updateGpsAdqTime(GpsRate gpsRate);

/**
 * @brief Enter low power sleep mode
 */
void enterLowPowerSleep();

// ============================================================================
// COW OPERATIONS
// ============================================================================

/**
 * @brief Check if cow is in green zone
 * @param cow Cow object to check
 * @return HAL_OK if in green zone, HAL_ERROR otherwise
 */
HAL_StatusTypeDef isInGreenZone(Cow& cow);

/**
 * @brief Update cow state from burst of IMU samples (PREFERRED METHOD)
 * 
 * Processes N raw accelerometer samples to:
 * - Compute burst features (E = avg deviation, peaks count)
 * - Classify cow state with dynamic thresholds
 * - Apply persistence and anti-flapping logic
 * - Auto-calibrate g² during quiet periods
 * - Detect sleep after X minutes of quiet
 * 
 * @param cow Cow object to update
 * @param samples Array of N raw accelerometer samples (axRaw, ayRaw, azRaw)
 * @param N Number of samples (typically 52 for 2s @ 26Hz)
 */
void updateStateFromBurst(Cow& cow, const AccRaw* samples, uint16_t N);

/**
 * @brief Update cow state based on current acceleration (DEPRECATED)
 * @deprecated Use updateStateFromBurst() for robust burst-based classification
 * @param cow Cow object to update
 */
void updateState(Cow& cow);

#endif // FSM_HELPER_H
