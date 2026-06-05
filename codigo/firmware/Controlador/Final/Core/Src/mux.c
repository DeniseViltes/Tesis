/*
 * mux.c
 *
 *  Created on: 3 jun 2026
 *      Author: ---
 */


#include "mux.h"

void MUX_Init(void)
{
    MUX_Select(0);
}

void MUX_Select(uint8_t channel)
{
    channel &= 0x07; // limita de 0 a 7

    HAL_GPIO_WritePin(S0_GPIO_Port, S0_Pin,
                      (channel & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    HAL_GPIO_WritePin(S1_GPIO_Port, S1_Pin,
                      (channel & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    HAL_GPIO_WritePin(S2_GPIO_Port, S2_Pin,
                      (channel & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
