
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

CowState classifyMotion(Acceleration imu)
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

zone_t getZoneForDistance(distance_t dist, Fence fence)
{
    for (int i = 0; i < BLACK_ZONE; i++)
    {
      if (dist < fence.getThresholds[i])
        return static_cast<zone_t>(i - 1);  // zona anterior
    }

    return BLACK_ZONE;
}

// TODO: Deberíamos pasar como argumento Cow *cow y Fence *fence o se hace mediante una QUEUE?
void sensorAcqTask(void *argument) {
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  Position gps;
  Acceleration imu;
  distance_t dist;
  zone_t zone; 
  CowState state;

  for (;;)
  {
    // === 1. Leer GPS ===
    if (GPS_ReadPosition(&gps) != HAL_OK)
    {
      gps_error_count++;
      continue;  // volver a intentar luego
    }

    cow.updatePosition(gps);

    // Mostrar GPS Read por UART
    printf("GPS read: lat=%.5f, lon=%.5f\r\n", gps.latitude, gps.longitude);

    // === 2. Determinar distancia a cerca virtual y zona ===
    dist = calculateDistanceToLimit(cow.getPosition(), fence.getSegments()); // TODO: Implementar esta función.
    zone = getZoneFromDistance(dist, fence); // GREEN, BLUE, YELLOW, RED, BLACK

    // Mostrar Zona por UART
    printf("Zone: %d, Distance: %.2f m\r\n", zone, dist);

    // === 3. Si no está en GREEN_ZONE, mandar estímulo ===
    if (zone != GREEN_ZONE)
    {
      // TODO: definir StimulusQueueHandle - ioc?
      xQueueSend(StimulusQueueHandle, &zone, 0); // Despierta StimulusTask
    }
    else
    {
      // === 4. Leer IMU ===
      if (LSM6DSO_ReadAccelGyro(&imu) == HAL_OK)
      {
        cow.updateAcceleration(imu);
        
        state = classifyMotion(imu);  // GRAZING, SLEEP, MOVEMENT

        // Mostrar IMU Read por UART
        printf("IMU: ax=%.2f, ay=%.2f, az=%.2f -> State: %d\r\n", imu.ax, imu.ay, imu.az, state);

        cow.updateState(state);

        // TODO: REVISAR para que funcione con CowState
        if (cow.getState() == SLEEP)
        {
          GPS_SetAcquisitionRate(VERY_SLOW);  // 1 muestra/hora
          enterLowPowerSleep();  // El micro se duerme, IMU genera WAKE_UP
        }
        elseif(cow.getState() == GRAZING)
        {
          GPS_SetAcquisitionRate(SLOW); // p.ej. 1 muestra/30 min
        }
        elseif(cow.getState() == MOVEMENT)
        {
          if (dist < NEAR_LIMIT)
            GPS_SetAcquisitionRate(MEDIUM);
          else
            GPS_SetAcquisitionRate(FAST);  // para saber si se acerca al límite
        }
      }
    }

    // TODO: REVISAR, queremos que esto varíe?? 
    osDelay(GPS_SAMPLE_RATE);  // p.ej. 1 vez por minuto o lo que hayas configurado
  }

  /* USER CODE END 5 */

}