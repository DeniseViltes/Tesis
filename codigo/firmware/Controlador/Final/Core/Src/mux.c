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




/*
 * Celda lógica -> canal físico del MUX
 *
 * celda 0 (cell_neg1) -> A6
 * celda 1 (cell_neg2) -> A4
 * celda 2 (cell_neg3) -> A7
 * celda 3 (cell_neg4) -> A5
 * celda 4 (cell_neg5) -> A2
 * celda 5 (cell_neg6) -> A1
 * celda 6 (cell_neg7) -> A0
 *
 * A3 -> GND
 */

void MUX_Select(mux_t *mux, uint8_t cell)
{
    static const uint8_t mux_map[7] = {
        6,  // cell 0 -> cell_neg1 -> A6
        4,  // cell 1 -> cell_neg2 -> A4
        7,  // cell 2 -> cell_neg3 -> A7
        5,  // cell 3 -> cell_neg4 -> A5
        2,  // cell 4 -> cell_neg5 -> A2
        1,  // cell 5 -> cell_neg6 -> A1
        0   // cell 6 -> cell_neg7 -> A0
    };

    if (cell >= 7)
        return;

    uint8_t pin = mux_map[cell];

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
}
