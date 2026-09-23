/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#define DATA2_Pin GPIO_PIN_13
#define DATA2_GPIO_Port GPIOC
#define medicion_corriente_de_carga_Pin GPIO_PIN_0
#define medicion_corriente_de_carga_GPIO_Port GPIOC
#define salida_adc_2_Pin GPIO_PIN_2
#define salida_adc_2_GPIO_Port GPIOC
#define banco2_med_Pin GPIO_PIN_3
#define banco2_med_GPIO_Port GPIOC
#define Salida_adc_1_Pin GPIO_PIN_0
#define Salida_adc_1_GPIO_Port GPIOA
#define banco1_med_Pin GPIO_PIN_1
#define banco1_med_GPIO_Port GPIOA
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define banco4_med_Pin GPIO_PIN_5
#define banco4_med_GPIO_Port GPIOA
#define medicion_corriente_descarga_Pin GPIO_PIN_6
#define medicion_corriente_descarga_GPIO_Port GPIOA
#define banco3_med_Pin GPIO_PIN_7
#define banco3_med_GPIO_Port GPIOA
#define salida_adc_3_Pin GPIO_PIN_4
#define salida_adc_3_GPIO_Port GPIOC
#define Salida_adc_4_Pin GPIO_PIN_5
#define Salida_adc_4_GPIO_Port GPIOC
#define S2_Pin GPIO_PIN_6
#define S2_GPIO_Port GPIOC
#define S1_Pin GPIO_PIN_8
#define S1_GPIO_Port GPIOC
#define S0_Pin GPIO_PIN_9
#define S0_GPIO_Port GPIOC
#define llave_de_emergencia_Pin GPIO_PIN_10
#define llave_de_emergencia_GPIO_Port GPIOA
#define DATA3_Pin GPIO_PIN_11
#define DATA3_GPIO_Port GPIOA
#define DATA4_Pin GPIO_PIN_12
#define DATA4_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define DATA1_Pin GPIO_PIN_15
#define DATA1_GPIO_Port GPIOA
#define CLK_Pin GPIO_PIN_10
#define CLK_GPIO_Port GPIOC
#define LATCH_Pin GPIO_PIN_11
#define LATCH_GPIO_Port GPIOC

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
