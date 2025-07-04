
#include "sensorAcqTask.h"

void enterLowPowerSleep(void)
{
  // Asegurarse de limpiar interrupciones previas
  __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

  // Deshabilitar SysTick para que no interrumpa durante el stop
  HAL_SuspendTick();

  // Habilitar wake-up por EXTI (ya debe estar configurado en NVIC / GPIO)
  // No es necesario hacerlo explícitamente si CubeMX lo generó bien

  // Entrar a modo STOP2
  //HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);

  // Entrar en modo SLEEP. El micro se detiene hasta que ocurra una interrupción.
  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

  // (El micro ahora está detenido. Al despertar vuelve desde aquí)

  // Reanudar SysTick para que FreeRTOS vuelva a funcionar
  HAL_ResumeTick();

  // Reconfigurar el reloj si hace falta
  SystemClock_Config();  // Necesario si usás HSE/HSEBYP/HSE+PLL
}

state_t classifyMotion(imu_data_t imu)
{
  float abs_ax = fabsf(imu.ax);
  float abs_ay = fabsf(imu.ay);
  float abs_az = fabsf(imu.az);

  if (abs_ax < 0.05f && abs_ay < 0.05f && abs_az < 0.05f)
    return SLEEP;
  else if (abs_ax < 0.05f && abs_ay < 0.05f && abs_az > 0.1f)
    return GRAZING;
  else
    return MOVEMENT;
}

void sensorAcqTask(void *argument) {
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  gps_data_t gps;
  zone_t zone;
  imu_data_t imu;
  distance_t dist;

  for (;;)
  {
    // === 1. Leer GPS ===
    if (GPS_ReadPosition(&gps) != HAL_OK)
    {
      gps_error_count++;
      continue;  // volver a intentar luego
    }

    // Mostrar GPS Read por UART
    printf("GPS read: lat=%.5f, lon=%.5f\r\n", gps.latitude, gps.longitude);

    // === 2. Determinar distancia a cerca virtual y zona ===
    dist = calculateDistanceToFence(gps); // función tuya
    zone = determineZoneFromDistance(dist); // BLUE, YELLOW, RED, GREEN

    // Mostrar Zona por UART
    printf("Zone: %d, Distance: %.2f m\r\n", zone, dist);

    // === 3. Si no está en GREEN_ZONE, mandar estímulo ===
    if (zone != GREEN_ZONE)
    {
      xQueueSend(StimulusQueueHandle, &zone, 0); // Despierta StimulusTask
    }
    else
    {
      // === 4. Leer IMU ===
      if (LSM6DSO_ReadAccelGyro(&imu) == HAL_OK)
      {
        // Mostrar IMU Read por UART
        printf("IMU: ax=%.2f, ay=%.2f, az=%.2f -> State: %d\r\n", imu.ax, imu.ay, imu.az, state);

        state_t state = classifyMotion(imu);  // GRAZING, SLEEP, MOVEMENT

        switch (state)
        {
          case GRAZING:
            GPS_SetAcquisitionRate(SLOW); // p.ej. 1 muestra/30 min
            break;

          case SLEEP:
            GPS_SetAcquisitionRate(VERY_SLOW);  // 1 muestra/hora
            enterLowPowerSleep();  // El micro se duerme, IMU genera WAKE_UP
            break;

          case MOVEMENT:
            if (dist < NEAR_LIMIT)
              GPS_SetAcquisitionRate(MEDIUM);
            else
              GPS_SetAcquisitionRate(FAST);  // para saber si se acerca al límite
            break;
        }
      }
    }

    osDelay(GPS_SAMPLE_RATE);  // p.ej. 1 vez por minuto o lo que hayas configurado
  }

  /* USER CODE END 5 */

}