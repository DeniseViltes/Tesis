#pragma once

#include "stm32f1xx_hal.h"
#include <ctype.h>


/* Inicializa la CLI (imprime banner y arranca RX por IT) */
void CLI_Init(UART_HandleTypeDef *huart);

/* Debe llamarse desde HAL_UART_RxCpltCallback */
void CLI_RxCallback(UART_HandleTypeDef *huart);

/* Boton azul para forzar modo manual */
void CLI_ButtonManualCallback(uint16_t gpio_pin);
