#pragma once

#include "stm32f1xx_hal.h"

/* Inicializa la CLI (imprime banner y arranca RX por IT) */
void CLI_Init(UART_HandleTypeDef *huart);

/* Debe llamarse desde HAL_UART_RxCpltCallback */
void CLI_RxCallback(UART_HandleTypeDef *huart);
