/**
 * @file  powerManager.c
 * @brief Power Manager implementation.
 *
 * Wake sources:
 *   - IMU INT1  → PC1 → EXTI line 1 (rising edge, active-high LSM6DSO INT1)
 *   - LPTIM1    → clocked by LSE (32 768 Hz / 128 = 256 Hz)
 *                 period = 256 * PM_BACKUP_WAKE_SECONDS ticks
 *
 * Post-STOP restore:
 *   - SystemClock_Config()  restores MSI 48 MHz
 *   - vcom_Resume()         restores USART2 + DMA1 channel 5 (trace/log UART)
 *
 * NOTE: Peripherals whose register contents are preserved in STOP (I2C2,
 *       USART1-GPS, TIM1/16/17) resume automatically after clock restore.
 */

#include "powerManager.h"
#include "usart_if.h"   /* vcom_Resume() – restores USART2 + DMA after STOP */

/* SystemClock_Config is implemented in main.c */
extern void SystemClock_Config(void);

/* =========================================================================
 * Module state
 * ========================================================================= */

/** LPTIM1 handle – also used by LPTIM1_IRQHandler in stm32wlxx_it.c */
LPTIM_HandleTypeDef powerManagerLptim1Handle;

static volatile WakeReason currentWakeReason = wakeReasonUnknown;
static volatile bool       imuWakeFlag       = false;
static volatile bool       lptimWakeFlag     = false;

static bool lptimArmed = false;
static bool extiArmed  = false;

/* =========================================================================
 * Private helpers
 * ========================================================================= */

/**
 * @brief Clear pending EXTI and LPTIM flags before entering STOP.
 *        Prevents spurious immediate wake-up.
 */
static void clearWakeFlags(void)
{
    /* Clear EXTI1 pending bit (PC1 = IMU INT1) */
    __HAL_GPIO_EXTI_CLEAR_IT(PM_IMU_WAKE_PIN);

    /* Clear LPTIM1 auto-reload match flag */
    if (lptimArmed)
    {
        __HAL_LPTIM_CLEAR_FLAG(&powerManagerLptim1Handle, LPTIM_FLAG_ARRM);
        __HAL_LPTIM_CLEAR_FLAG(&powerManagerLptim1Handle, LPTIM_FLAG_ARROK);
        __HAL_LPTIM_CLEAR_FLAG(&powerManagerLptim1Handle, LPTIM_FLAG_CMPM);
    }
}

/* =========================================================================
 * Public functions
 * ========================================================================= */

void powerManagerInit(void)
{
    currentWakeReason = wakeReasonUnknown;
    imuWakeFlag       = false;
    lptimWakeFlag     = false;
    lptimArmed        = false;
    extiArmed         = false;
}

/* -------------------------------------------------------------------------
 * powerManagerArmWakeSources
 * ------------------------------------------------------------------------- */
void powerManagerArmWakeSources(void)
{
    /* ---- 1. IMU wake via EXTI on PC1 (LSM6DSO INT1, active-high) -------- */
    if (!extiArmed)
    {
        __HAL_RCC_GPIOC_CLK_ENABLE();

        GPIO_InitTypeDef gpioInit = {0};
        gpioInit.Pin   = PM_IMU_WAKE_PIN;
        gpioInit.Mode  = GPIO_MODE_IT_RISING;   /* LSM6DSO INT1 asserts high */
        gpioInit.Pull  = GPIO_PULLDOWN;
        HAL_GPIO_Init(PM_IMU_WAKE_GPIO_PORT, &gpioInit);

        /* Clear stale pending bit before enabling NVIC */
        __HAL_GPIO_EXTI_CLEAR_IT(PM_IMU_WAKE_PIN);

        HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(EXTI1_IRQn);
        extiArmed = true;
    }

    /* ---- 2. LPTIM1 backup wake (continuous mode, ~PM_BACKUP_WAKE_SECONDS) */
    if (!lptimArmed)
    {
        /* Route LPTIM1 clock to LSE so it keeps running in STOP mode.
         * LSE = 32 768 Hz; prescaler /128 → tick rate = 256 Hz */
        RCC_PeriphCLKInitTypeDef periphClk = {0};
        periphClk.PeriphClockSelection  = RCC_PERIPHCLK_LPTIM1;
        periphClk.Lptim1ClockSelection  = RCC_LPTIM1CLKSOURCE_LSE;
        HAL_RCCEx_PeriphCLKConfig(&periphClk);

        __HAL_RCC_LPTIM1_CLK_ENABLE();

        powerManagerLptim1Handle.Instance             = LPTIM1;
        powerManagerLptim1Handle.Init.Clock.Source    = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
        powerManagerLptim1Handle.Init.Clock.Prescaler = LPTIM_PRESCALER_DIV128;
        powerManagerLptim1Handle.Init.Trigger.Source  = LPTIM_TRIGSOURCE_SOFTWARE;
        powerManagerLptim1Handle.Init.OutputPolarity  = LPTIM_OUTPUTPOLARITY_HIGH;
        powerManagerLptim1Handle.Init.UpdateMode      = LPTIM_UPDATE_IMMEDIATE;
        powerManagerLptim1Handle.Init.CounterSource   = LPTIM_COUNTERSOURCE_INTERNAL;
        HAL_LPTIM_Init(&powerManagerLptim1Handle);

        /* Auto-reload value: period = (32768/128) * backupWakeSeconds - 1
         *   = 256 * 60 - 1 = 15 359 for 60 s                              */
        uint32_t period = (32768U / 128U) * PM_BACKUP_WAKE_SECONDS - 1U;
        if (period > 0xFFFFU) period = 0xFFFFU;   /* clamp to 16-bit LPTIM ARR */

        HAL_NVIC_SetPriority(LPTIM1_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(LPTIM1_IRQn);

        /* Start counter in continuous mode – ARRM fires every 'period+1' ticks.
         * DisarmWakeSources() stops it after the first useful wake.         */
        HAL_LPTIM_Counter_Start_IT(&powerManagerLptim1Handle, period);
        lptimArmed = true;
    }
}

/* -------------------------------------------------------------------------
 * powerManagerDisarmWakeSources
 * ------------------------------------------------------------------------- */
void powerManagerDisarmWakeSources(void)
{
    if (lptimArmed)
    {
        HAL_LPTIM_Counter_Stop_IT(&powerManagerLptim1Handle);
        HAL_NVIC_DisableIRQ(LPTIM1_IRQn);
        __HAL_RCC_LPTIM1_CLK_DISABLE();
        lptimArmed = false;
    }

    if (extiArmed)
    {
        HAL_NVIC_DisableIRQ(EXTI1_IRQn);

        /* Reconfigure PC1 as high-impedance input to minimise leakage */
        GPIO_InitTypeDef gpioInit = {0};
        gpioInit.Pin  = PM_IMU_WAKE_PIN;
        gpioInit.Mode = GPIO_MODE_INPUT;
        gpioInit.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(PM_IMU_WAKE_GPIO_PORT, &gpioInit);

        extiArmed = false;
    }
}

/* -------------------------------------------------------------------------
 * powerManagerEnterStop
 * ------------------------------------------------------------------------- */
void powerManagerEnterStop(void)
{
    /* 1. Clear any stale wake flags to avoid immediate spurious wake */
    clearWakeFlags();

    /* 2. Disable IPCC RX/TX interrupts before entering STOP.
     *
     *    The CM0+ LoRaWAN stack runs independently and fires IPCC events for
     *    every uplink confirmation, downlink, or MAC timer.  Those events are
     *    the primary cause of spurious wakes: without this mask the CM4 exits
     *    STOP almost immediately every time the CM0+ has activity.
     *
     *    Safety: masking the NVIC does NOT discard the pending IPCC flag in
     *    hardware.  When we call HAL_NVIC_EnableIRQ() after waking, the
     *    interrupt fires at once and the mbmux processes all queued CM0+
     *    messages – no LoRaWAN events are lost.                              */
    HAL_NVIC_DisableIRQ(IPCC_C1_RX_IRQn);
    HAL_NVIC_DisableIRQ(IPCC_C1_TX_IRQn);

    /* 3. Suspend HAL tick source (TIM or SysTick) */
    HAL_SuspendTick();

    /* 4. Enter STOP mode – CPU halts here until a wake source fires ----------
     *    Only EXTI lines and LPTIM1 (armed above) can wake us now.
     *    Execution resumes at the line immediately after this call.          */
#if PM_STOP_MODE == 2
    HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
#else
    HAL_PWREx_EnterSTOP1Mode(PWR_STOPENTRY_WFI);
#endif
    /* ---- CPU resumes here after wake --------------------------------------- */

    /* 5. Restore HAL tick */
    HAL_ResumeTick();

    /* 6. Restore system clocks.
     *    In STOP mode, MSI (SYSCLK source) is stopped.  SystemClock_Config()
     *    re-enables MSI at 48 MHz and re-locks APB/AHB dividers.            */
    SystemClock_Config();

    /* 7. Restore USART2 + DMA1 ch5 (trace/log UART – not retained in STOP).
     *    vcom_Resume() re-runs HAL_UART_Init + HAL_DMA_Init for huart2.     */
    vcom_Resume();

    /* 8. Re-enable IPCC RX/TX so the LoRaWAN stack catches up.
     *    Any pending CM0+ flag triggers the ISR immediately here.           */
    HAL_NVIC_EnableIRQ(IPCC_C1_RX_IRQn);
    HAL_NVIC_EnableIRQ(IPCC_C1_TX_IRQn);

    /* 9. Determine wake reason from flags set in the ISR callbacks */
    if (imuWakeFlag)
    {
        currentWakeReason = wakeReasonImu;
    }
    else if (lptimWakeFlag)
    {
        currentWakeReason = wakeReasonLptim;
    }
    else
    {
        currentWakeReason = wakeReasonUnknown;
    }
}

/* -------------------------------------------------------------------------
 * powerManagerGetWakeReason / powerManagerClearWakeReason
 * ------------------------------------------------------------------------- */
WakeReason powerManagerGetWakeReason(void)
{
    return currentWakeReason;
}

void powerManagerClearWakeReason(void)
{
    currentWakeReason = wakeReasonUnknown;
    imuWakeFlag       = false;
    lptimWakeFlag     = false;
}

/* =========================================================================
 * HAL weak-symbol overrides (ISR callbacks)
 * ========================================================================= */

/**
 * @brief  Called by HAL_GPIO_EXTI_IRQHandler when PC1 fires.
 *         Sets the IMU wake flag and latches the wake reason.
 *         Runs in IRQ context – keep minimal.
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == PM_IMU_WAKE_PIN)
    {
        imuWakeFlag       = true;
        currentWakeReason = wakeReasonImu;
    }
}

/**
 * @brief  Called by HAL_LPTIM_IRQHandler when LPTIM1 auto-reload matches.
 *         Sets the LPTIM wake flag and latches the wake reason.
 *         The timer continues in continuous mode; DisarmWakeSources() stops it.
 *         Runs in IRQ context – keep minimal.
 */
void HAL_LPTIM_AutoReloadMatchCallback(LPTIM_HandleTypeDef *hlptim)
{
    if (hlptim->Instance == LPTIM1)
    {
        lptimWakeFlag     = true;
        currentWakeReason = wakeReasonLptim;
    }
}
