/*
 * mux.c
 *
 *  Created on: 3 jun 2026
 *      Author: ---
 */


#include "mux.h"


void MUX_Init(mux_t *mux, GPIO_TypeDef *s0_port, uint16_t s0_pin,
			GPIO_TypeDef *s1_port, uint16_t s1_pin,
			 GPIO_TypeDef *s2_port, uint16_t  s2_pin)
{
	mux->s0_port =s0_port ;
	mux->s0_pin = s0_pin;

	mux->s1_port = s1_port;
	mux->s1_pin = s1_pin;

	mux->s2_port =s2_port;
	mux->s2_pin = s2_pin;

	MUX_Select(mux, 3);//gnd

	mux->canal_seleccionado = 3;


}


void MUX_Select(mux_t *mux, uint8_t pin)
{
	if (mux == NULL || pin > 7u) return;


    HAL_GPIO_WritePin(
        mux->s0_port,
        mux->s0_pin,
        (pin & 0x01u) ? GPIO_PIN_SET : GPIO_PIN_RESET
    );

    HAL_GPIO_WritePin(
        mux->s1_port,
        mux->s1_pin,
        (pin & 0x02u) ? GPIO_PIN_SET : GPIO_PIN_RESET
    );

    HAL_GPIO_WritePin(
        mux->s2_port,
        mux->s2_pin,
        (pin & 0x04u) ? GPIO_PIN_SET : GPIO_PIN_RESET
    );
    mux->canal_seleccionado = pin;
}
