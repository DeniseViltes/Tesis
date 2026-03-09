/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Modo_manual_Pin GPIO_PIN_13
#define Modo_manual_GPIO_Port GPIOC
#define Modo_manual_EXTI_IRQn EXTI15_10_IRQn
#define cell_31_Pin GPIO_PIN_0
#define cell_31_GPIO_Port GPIOC
#define cell_32_Pin GPIO_PIN_1
#define cell_32_GPIO_Port GPIOC
#define cell_11_Pin GPIO_PIN_0
#define cell_11_GPIO_Port GPIOA
#define cell_12_Pin GPIO_PIN_1
#define cell_12_GPIO_Port GPIOA
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define cell_21_Pin GPIO_PIN_4
#define cell_21_GPIO_Port GPIOA
#define Vc_11_Pin GPIO_PIN_5
#define Vc_11_GPIO_Port GPIOA
#define out_neg_Pin GPIO_PIN_6
#define out_neg_GPIO_Port GPIOA
#define out_pos_Pin GPIO_PIN_7
#define out_pos_GPIO_Port GPIOA
#define bus_2_Pin GPIO_PIN_4
#define bus_2_GPIO_Port GPIOC
#define cell_22_Pin GPIO_PIN_0
#define cell_22_GPIO_Port GPIOB
#define Vc_31_Pin GPIO_PIN_10
#define Vc_31_GPIO_Port GPIOB
#define Vc_32_Pin GPIO_PIN_11
#define Vc_32_GPIO_Port GPIOB
#define Vc_12_Pin GPIO_PIN_12
#define Vc_12_GPIO_Port GPIOB
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define Vc_1_Pin GPIO_PIN_3
#define Vc_1_GPIO_Port GPIOB
#define Vc_2_Pin GPIO_PIN_4
#define Vc_2_GPIO_Port GPIOB
#define Vc_3_Pin GPIO_PIN_5
#define Vc_3_GPIO_Port GPIOB
#define otro_Pin GPIO_PIN_6
#define otro_GPIO_Port GPIOB
#define Vc_21_Pin GPIO_PIN_8
#define Vc_21_GPIO_Port GPIOB
#define Vc_22_Pin GPIO_PIN_9
#define Vc_22_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
