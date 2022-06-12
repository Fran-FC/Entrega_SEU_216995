/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <FreeRTOS.h>
#include <stdint.h>
#include <semphr.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
enum sensors {LDR=0,NTC=1};

enum modes {
	local=0,
	connected=1,
	clon=2,
	test=3
};

enum cipstates {
	ap_disconnected = 5,
	ap_connected = 2,
	socket_opened = 3
};

struct GlobalStatus {
	uint8_t CURRENT_CIPSTATE;
	uint8_t CURRENT_MODE;
	uint8_t CURRENT_SENSOR;
	uint8_t buzz_flag;
	uint8_t clon_status_code;

	float temps[4];

	float lums[4];

	char alarm_src[40];

	SemaphoreHandle_t xSem;
};


/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

uint32_t readAnalogChannel(unsigned int channel);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define LDR_Pin GPIO_PIN_0
#define LDR_GPIO_Port GPIOA
#define NTC_Pin GPIO_PIN_1
#define NTC_GPIO_Port GPIOA
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define Pot_Pin GPIO_PIN_4
#define Pot_GPIO_Port GPIOA
#define Led8_Pin GPIO_PIN_5
#define Led8_GPIO_Port GPIOA
#define Led6_Pin GPIO_PIN_6
#define Led6_GPIO_Port GPIOA
#define Buzzer_Pin GPIO_PIN_7
#define Buzzer_GPIO_Port GPIOA
#define Led7_Pin GPIO_PIN_0
#define Led7_GPIO_Port GPIOB
#define Led2_Pin GPIO_PIN_10
#define Led2_GPIO_Port GPIOB
#define LeftButton_Pin GPIO_PIN_7
#define LeftButton_GPIO_Port GPIOC
#define Led3_Pin GPIO_PIN_8
#define Led3_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define Led5_Pin GPIO_PIN_3
#define Led5_GPIO_Port GPIOB
#define Led1_Pin GPIO_PIN_4
#define Led1_GPIO_Port GPIOB
#define Led4_Pin GPIO_PIN_5
#define Led4_GPIO_Port GPIOB
#define RightButton_Pin GPIO_PIN_6
#define RightButton_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
