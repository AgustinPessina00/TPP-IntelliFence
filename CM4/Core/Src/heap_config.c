/**
  ******************************************************************************
  * @file           : heap_config.c
  * @brief          : FreeRTOS heap configuration for RAM2 usage
  ******************************************************************************
  * @attention
  *
  * This file configures FreeRTOS heap_5 to use RAM2 (backup SRAM)
  * for better memory management in STM32WL55JC dual-core system.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "portable.h"
#include <stdio.h>

/* Private defines -----------------------------------------------------------*/
#define RAM2_START_ADDRESS   0x20008000U
#define RAM2_TOTAL_SIZE      (16 * 1024U)     /* 16KB total RAM2 */
#define RAM2_HEAP_SIZE       (8 * 1024U)      /* 8KB for FreeRTOS heap */
#define RAM2_REMAINING_START (RAM2_START_ADDRESS + RAM2_HEAP_SIZE)
#define RAM2_REMAINING_SIZE  (RAM2_TOTAL_SIZE - RAM2_HEAP_SIZE)

/* Private variables ---------------------------------------------------------*/

/* Heap regions definition for heap_5 */
static const HeapRegion_t xHeapRegions[] = {
    /* Primary heap region in RAM2 - must be first (lowest address) */
    { 
        .pucStartAddress = (uint8_t*)RAM2_START_ADDRESS, 
        .xSizeInBytes = RAM2_HEAP_SIZE 
    },
    
    /* Optional: Add RAM1 as secondary region if needed 
    { 
        .pucStartAddress = (uint8_t*)0x20000000, 
        .xSizeInBytes = (4 * 1024)  // Use 4KB from RAM1 if needed
    },
    */
    
    /* Terminate the array */
    { NULL, 0 }
};

/* Public Functions ----------------------------------------------------------*/

/**
  * @brief  Initialize FreeRTOS heap regions to use RAM2
  * @note   This function MUST be called before any FreeRTOS API calls
  *         including osKernelInitialize(), task creation, etc.
  * @retval None
  */
void vApplicationSetupHeap(void)
{
    /* Configure heap regions for heap_5 */
    vPortDefineHeapRegions(xHeapRegions);
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
    
    printf("[HEAP] Total: %u bytes, Free: %u bytes, Min Ever Free: %u bytes\n", 
           (unsigned int)xTotalSize, (unsigned int)xFreeSize, (unsigned int)xMinEverFree);
    printf("[HEAP] Located in RAM2 (0x%08X - 0x%08X)\n", 
           RAM2_START_ADDRESS, RAM2_START_ADDRESS + RAM2_HEAP_SIZE - 1);
}