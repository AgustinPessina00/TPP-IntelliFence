/**
  ******************************************************************************
  * @file           : heap_config.c
  * @brief          : FreeRTOS heap_4 configuration for RAM1 usage
  ******************************************************************************
  * @attention
  *
  * This file configures FreeRTOS heap_4 to use RAM1
  * for memory management in STM32WL55JC dual-core system.
  *
  * heap_4 automatically manages the heap - no initialization needed.
  * Just define the ucHeap array with configTOTAL_HEAP_SIZE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "portable.h"
#include <stdio.h>
#include <stdint.h>

/* Private defines -----------------------------------------------------------*/
#define HEAP_SIZE            configTOTAL_HEAP_SIZE

/* Private variables ---------------------------------------------------------*/

/**
 * @brief FreeRTOS heap_4 memory pool in RAM1
 * @note heap_4 automatically finds and uses this array
 *       The .heap section is placed in RAM1 by the linker script
 */
__attribute__((section(".heap"))) __attribute__((used))
uint8_t ucHeap[configTOTAL_HEAP_SIZE];

/* Public Functions ----------------------------------------------------------*/

/**
  * @brief  Get heap statistics for debugging
  * @param  pxTotalHeapSize: Pointer to store total heap size
  * @param  pxFreeHeapSize: Pointer to store current free heap size
  * @param  pxMinimumEverFreeHeapSize: Pointer to store minimum free size
  * @retval None
  */
void vGetHeapStats(size_t *pxTotalHeapSize, size_t *pxFreeHeapSize, size_t *pxMinimumEverFreeHeapSize)
{
    if (pxTotalHeapSize != NULL) {
        *pxTotalHeapSize = HEAP_SIZE;
    }
    
    if (pxFreeHeapSize != NULL) {
        *pxFreeHeapSize = xPortGetFreeHeapSize();
    }
    
    if (pxMinimumEverFreeHeapSize != NULL) {
        *pxMinimumEverFreeHeapSize = xPortGetMinimumEverFreeHeapSize();
    }
}

/**
  * @brief  Print heap information to debug console
  * @retval None
  */
void vPrintHeapInfo(void)
{
    size_t xTotalSize, xFreeSize, xMinEverFree;
    vGetHeapStats(&xTotalSize, &xFreeSize, &xMinEverFree);
    
    printf("[HEAP] Total: %u bytes, Free: %u bytes, Min Ever Free: %u bytes\r\n", 
           (unsigned int)xTotalSize, (unsigned int)xFreeSize, (unsigned int)xMinEverFree);
    printf("[HEAP] Heap location: 0x%08X\r\n", (unsigned int)ucHeap);
}

/**
  * @brief  Get heap statistics for debugging
  * @param  pxTotalHeapSize: Pointer to store total heap size
  * @param  pxFreeHeapSize: Pointer to store current free heap size
  * @param  pxMinimumEverFreeHeapSize: Pointer to store minimum free size
  * @retval None
  */
void vGetHeapStats(size_t *pxTotalHeapSize, size_t *pxFreeHeapSize, size_t *pxMinimumEverFreeHeapSize)
{
    if (pxTotalHeapSize != NULL) {
        *pxTotalHeapSize = RAM2_HEAP_SIZE;
    }
    
    if (pxFreeHeapSize != NULL) {
        *pxFreeHeapSize = xPortGetFreeHeapSize();
    }
    
    if (pxMinimumEverFreeHeapSize != NULL) {
        *pxMinimumEverFreeHeapSize = xPortGetMinimumEverFreeHeapSize();
    }
}

/**
  * @brief  Print heap information to debug console
  * @retval None
  */
void vPrintHeapInfo(void)
{
    size_t xTotalSize, xFreeSize, xMinEverFree;
    vGetHeapStats(&xTotalSize, &xFreeSize, &xMinEverFree);
    
    printf("[HEAP] Total: %u bytes, Free: %u bytes, Min Ever Free: %u bytes\r\n", 
           (unsigned int)xTotalSize, (unsigned int)xFreeSize, (unsigned int)xMinEverFree);
    printf("[HEAP] Located in RAM2 (0x%08X - 0x%08X)\r\n", 
           RAM2_START_ADDRESS, RAM2_START_ADDRESS + RAM2_HEAP_SIZE - 1);
}


/**
  * @brief  Get heap statistics for debugging
  * @param  pxTotalHeapSize: Pointer to store total heap size
  * @param  pxFreeHeapSize: Pointer to store current free heap size
  * @param  pxMinimumEverFreeHeapSize: Pointer to store minimum free size
  * @retval None
  */
void vGetHeapStats(size_t *pxTotalHeapSize, size_t *pxFreeHeapSize, size_t *pxMinimumEverFreeHeapSize)
{
    if (pxTotalHeapSize != NULL) {
        *pxTotalHeapSize = RAM2_HEAP_SIZE;
    }
    
    if (pxFreeHeapSize != NULL) {
        *pxFreeHeapSize = xPortGetFreeHeapSize();
    }
    
    if (pxMinimumEverFreeHeapSize != NULL) {
        *pxMinimumEverFreeHeapSize = xPortGetMinimumEverFreeHeapSize();
    }
}

/**
  * @brief  Print heap information to debug console
  * @retval None
  */
void vPrintHeapInfo(void)
{
    size_t xTotalSize, xFreeSize, xMinEverFree;
    vGetHeapStats(&xTotalSize, &xFreeSize, &xMinEverFree);
    
    printf("[HEAP] Total: %u bytes, Free: %u bytes, Min Ever Free: %u bytes\r\n", 
           (unsigned int)xTotalSize, (unsigned int)xFreeSize, (unsigned int)xMinEverFree);
    printf("[HEAP] Located in RAM2 (0x%08X - 0x%08X)\r\n", 
           RAM2_START_ADDRESS, RAM2_START_ADDRESS + RAM2_HEAP_SIZE - 1);
}