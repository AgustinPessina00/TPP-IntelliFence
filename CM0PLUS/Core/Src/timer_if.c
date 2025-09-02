// timer_if_cm0.c – implementación para CM0+ SIN RTC
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>
#include <stdbool.h>
#include "timer_if.h"   // tu header
// NO incluir stm32wlxx_hal_rtc.h
// NO declarar extern RTC_HandleTypeDef hrtc

// ---- Utilidades internas basadas en ticks ----
static inline uint32_t ticks_now(void) {
    return (uint32_t)xTaskGetTickCount();
}
static inline uint32_t ms_to_ticks(uint32_t ms) {
    // redondeo hacia arriba para no subestimar
    uint64_t t = ((uint64_t)ms * configTICK_RATE_HZ + 999ULL) / 1000ULL;
    if (t == 0) t = 1;
    return (uint32_t)t;
}
static inline uint32_t ticks_to_ms(uint32_t t) {
    return (uint32_t)((uint64_t)t * 1000ULL / configTICK_RATE_HZ);
}

// ---- Estado local equivalente al "contexto" ----
static bool     s_inited = false;
static uint32_t s_ctx_ticks = 0;

// ---- Implementación de la MISMA API ----
UTIL_TIMER_Status_t TIMER_IF_Init(void)
{
    // En CM0 no tocamos RTC. Sólo marcamos init.
    s_inited = true;
    return UTIL_TIMER_OK;
}

UTIL_TIMER_Status_t TIMER_IF_StartTimer(uint32_t timeout_ticks)
{
    // En CM0 no programamos alarmas de RTC.
    // Si alguna librería llama a esto esperando un “armado” de timer,
    // lo dejamos como NO-OP y devolvemos OK (o implementá un sw-timer si lo necesitás).
    (void)timeout_ticks;
    return UTIL_TIMER_OK;
}

UTIL_TIMER_Status_t TIMER_IF_StopTimer(void)
{
    // No hay alarma RTC que desactivar en CM0.
    return UTIL_TIMER_OK;
}

uint32_t TIMER_IF_SetTimerContext(void)
{
    s_ctx_ticks = ticks_now();
    return s_ctx_ticks;
}

uint32_t TIMER_IF_GetTimerContext(void)
{
    return s_ctx_ticks;
}

uint32_t TIMER_IF_GetTimerElapsedTime(void)
{
    // Diferencia de ticks desde el contexto
    return (uint32_t)(ticks_now() - s_ctx_ticks);
}

uint32_t TIMER_IF_GetTimerValue(void)
{
    // Valor “absoluto” de tiempo para esta implementación: ticks actuales
    if (!s_inited) return 0;
    return ticks_now();
}

uint32_t TIMER_IF_GetMinimumTimeout(void)
{
    // Mínimo “timeout” manejable en este esquema → 1 tick
    return 1;
}

void TIMER_IF_DelayMs(uint32_t delay_ms)
{
    vTaskDelay(ms_to_ticks(delay_ms));
}

uint32_t TIMER_IF_Convert_ms2Tick(uint32_t timeMilliSec)
{
    return ms_to_ticks(timeMilliSec);
}

uint32_t TIMER_IF_Convert_Tick2ms(uint32_t tick)
{
    return ticks_to_ms(tick);
}

uint32_t TIMER_IF_GetTime(uint16_t *mSeconds)
{
    // En CM0 no devolvemos “segundos RTC” reales.
    // Emulamos: segundos/“mseg parciales” a partir del tick de FreeRTOS.
    // Esto mantiene compatibilidad de firma sin tocar RTC.
    const uint32_t t = ticks_now();
    const uint32_t ms = ticks_to_ms(t);
    if (mSeconds) *mSeconds = (uint16_t)(ms % 1000U);
    return ms / 1000U;
}

// Stub de backup registers (no hay RTC en CM0)
void     TIMER_IF_BkUp_Write_Seconds(uint32_t Seconds)     { (void)Seconds; }
uint32_t TIMER_IF_BkUp_Read_Seconds(void)                  { return 0; }
void     TIMER_IF_BkUp_Write_SubSeconds(uint32_t SubSec)   { (void)SubSec; }
uint32_t TIMER_IF_BkUp_Read_SubSeconds(void)               { return 0; }
