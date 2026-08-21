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

	mux->canal_seleccionado = 0;


}

void MUX_SetNodo (mux_t *mux,adc_node_t nodo){
	mux->nodo = nodo;
}




void MUX_Select(mux_t *mux, uint8_t pin)
{
    pin &= 0x07; // limita de 0 a 7

    HAL_GPIO_WritePin(mux->s0_port, mux->s0_pin,
                      (pin & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    HAL_GPIO_WritePin(mux->s1_port, mux->s1_pin,
                      (pin & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    HAL_GPIO_WritePin(mux->s2_port, mux->s2_pin,
                      (pin & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

