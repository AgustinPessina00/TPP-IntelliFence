/**
 * @file  powerManager.h
 * @brief Power Manager – deep-sleep (STOP1/STOP2) with IMU EXTI + LPTIM1 backup wake.
 *
 * Usage from FSM (cowSleeping state):
 *   powerManagerArmWakeSources();
 *   powerManagerClearWakeReason();
 *   powerManagerEnterStop();           // blocks until wake
 *   powerManagerDisarmWakeSources();
 *   WakeReason r = powerManagerGetWakeReason();
 *   if (r == wakeReasonImu)  -> re-evaluate cow state
 *   else                     -> LPTIM health-check, re-enter sleep if still sleeping
 */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include "stm32wlxx_hal.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Types
 * ========================================================================= */

/** Wake reason reported after returning from powerManagerEnterStop() */
typedef enum {
    wakeReasonUnknown = 0,  /**< Could not determine source (should not happen) */
    wakeReasonImu,          /**< IMU INT1 fired on PC1 (EXTI line 1) */
    wakeReasonRtc,          /**< RTC alarm (not currently used, reserved) */
    wakeReasonLptim         /**< LPTIM1 backup timer expired */
} WakeReason;

/* =========================================================================
 * Configuration
 * ========================================================================= */

/** STOP mode: 1 = STOP1 (main regulator low-power), 2 = STOP2 (lower power) */
#define PM_STOP_MODE            2

/** LPTIM1 one-shot period in seconds (backup wake if IMU never fires) */
#define PM_BACKUP_WAKE_SECONDS  60U

/* =========================================================================
 * IMU wake pin – PC1
 * The LSM6DSO INT1 output is connected to PC1 on this board.
 * PC1 → EXTI line 1 → EXTI1_IRQn (IRQ #7 on CM4).
 * ========================================================================= */
#define PM_IMU_WAKE_PIN         GPIO_PIN_1
#define PM_IMU_WAKE_GPIO_PORT   GPIOC

/* =========================================================================
 * LPTIM1 handle exposed for IRQ dispatcher in stm32wlxx_it.c
 * ========================================================================= */
extern LPTIM_HandleTypeDef powerManagerLptim1Handle;

/* =========================================================================
 * Public API
 * ========================================================================= */

/**
 * @brief  Initialize power manager state (call once at startup).
 */
void powerManagerInit(void);

/**
 * @brief  Configure and enable wake sources (EXTI on PC1 + LPTIM1 backup timer).
 *         Idempotent: safe to call if already armed.
 */
void powerManagerArmWakeSources(void);

/**
 * @brief  Disable wake sources (NVIC + LPTIM1 stopped).
 *         Call after every wake to allow clean re-arm on next sleep entry.
 */
void powerManagerDisarmWakeSources(void);

/**
 * @brief  Enter STOP mode.
 *         Sequence:
 *           1. Clear stale EXTI/LPTIM flags
 *           2. HAL_SuspendTick()
 *           3. HAL_PWREx_EnterSTOPxMode(WFI)  ← CPU halts here
 *           4. HAL_ResumeTick()
 *           5. SystemClock_Config()            ← restore MSI
 *           6. vcom_Resume()                   ← restore USART2+DMA
 *           7. Determine and store wake reason
 */
void powerManagerEnterStop(void);

/**
 * @brief  Return the reason the CPU woke from STOP.
 *         Valid after powerManagerEnterStop() returns.
 */
WakeReason powerManagerGetWakeReason(void);

/**
 * @brief  Clear wake reason and internal flags.
 *         Call before powerManagerEnterStop() for a clean state.
 */
void powerManagerClearWakeReason(void);

#ifdef __cplusplus
}
#endif

#endif /* POWER_MANAGER_H */
