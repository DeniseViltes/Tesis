/*
 * mux.h
 *
 *  Created on: 3 jun 2026
 *      Author: ---
 */

#ifndef INC_MUX_H_
#define INC_MUX_H_

#include "main.h"
#include "adc.h"

/*
 * A0->cell_neg7
 * A1->cell_neg6
 * A2->cell_neg5
 * A3->gnd
 * A4->cell_neg2
 * A5->cell_neg4
 * A6->cell_neg1
 * A7->cell_neg3
 */



typedef struct {
    GPIO_TypeDef *s0_port;
    uint16_t      s0_pin;

    GPIO_TypeDef *s1_port;
    uint16_t      s1_pin;

    GPIO_TypeDef *s2_port;
    uint16_t      s2_pin;

    uint8_t canal_seleccionado;
    adc_node_t nodo;//VER OTRA ALTERNATIVA
} mux_t;


void MUX_Init(mux_t *mux, GPIO_TypeDef *s0_port, uint16_t s0_pin,
			GPIO_TypeDef *s1_port, uint16_t s1_pin,
			 GPIO_TypeDef *s2_port, uint16_t  s2_pin);
void MUX_Select(mux_t *mux, uint8_t pin);

void MUX_SetNodo (mux_t *mux,adc_node_t nodo);


#endif /* INC_MUX_H_ */
