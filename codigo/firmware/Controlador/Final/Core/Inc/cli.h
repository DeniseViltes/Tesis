/*
 * cli.h
 *
 *  Created on: 12 jun 2026
 *      Author: ---
 */

#ifndef INC_CLI_H_
#define INC_CLI_H_

#include "stm32f1xx_hal.h"
#include <ctype.h>


//Me copio del CLI del Prototipo

/* Inicializa la CLI (imprime banner y arranca RX por IT) */
void CLI_Init(UART_HandleTypeDef *huart);

/* Debe llamarse desde HAL_UART_RxCpltCallback */
void CLI_RxCallback(UART_HandleTypeDef *huart);

/* Boton azul para forzar modo manual */
void CLI_ButtonReiniciarCallback(uint16_t gpio_pin);
void CLI_Process(void);

#endif /* INC_CLI_H_ */
