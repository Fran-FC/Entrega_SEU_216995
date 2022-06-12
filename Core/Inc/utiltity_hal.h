/*
 * utiltity_hal.h
 *
 *  Created on: May 3, 2022
 *      Author: pperez
 */

#ifndef INC_UTILTITY_HAL_H_
#define INC_UTILTITY_HAL_H_
#include "FreeRTOS.h"
#include <stdio.h>
#include "tareas_serie.h"
#include <task.h>
#include "cmsis_os.h"
#include "main.h"
#include "utility.h"
#include <string.h>
#include <stm32f4xx_hal_dma.h>

unsigned portBASE_TYPE makeFreeRtosPriority (osPriority priority);
static void DMA_SetConfig(DMA_HandleTypeDef *hdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength);
static void UART_EndRxTransfer(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_DMA_Abort_PAS(DMA_HandleTypeDef *hdma);
HAL_StatusTypeDef HAL_UART_DMAStop_PAS(UART_HandleTypeDef *huart);

#endif /* INC_UTILTITY_HAL_H_ */
