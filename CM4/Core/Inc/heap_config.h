/**
  ******************************************************************************
  * @file           : heap_config.h
  * @brief          : Header for heap_config.c file
  ******************************************************************************
  * @attention
  *
  * This file contains the declarations for FreeRTOS heap configuration
  * functions that setup heap_5 to use RAM2 (backup SRAM).
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __HEAP_CONFIG_H
#define __HEAP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stddef.h>

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  Initialize FreeRTOS heap regions to use RAM2
  * @note   This function MUST be called before any FreeRTOS API calls
  * @retval None
  */
// void vApplicationSetupHeap(void);  // Not needed for heap_4

/**
  * @brief  Get heap statistics for debugging
  * @param  pxTotalHeapSize: Pointer to store total heap size
  * @param  pxFreeHeapSize: Pointer to store current free heap size  
  * @param  pxMinimumEverFreeHeapSize: Pointer to store minimum free size
  * @retval None
  */
void vGetHeapStats(size_t *pxTotalHeapSize, size_t *pxFreeHeapSize, size_t *pxMinimumEverFreeHeapSize);

/**
  * @brief  Print heap information to debug console
  * @retval None
  */
void vPrintHeapInfo(void);

#ifdef __cplusplus
}
#endif

#endif /* __HEAP_CONFIG_H */