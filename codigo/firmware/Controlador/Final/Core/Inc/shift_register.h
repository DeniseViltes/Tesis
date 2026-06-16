// shift_register.h

#ifndef SHIFT_REGISTER_H
#define SHIFT_REGISTER_H

#include "main.h"
#include <stdint.h>

// Pines definidos en CubeMX/main.h
// Ejemplo:
// #define SR_DATA_Pin GPIO_PIN_0
// #define SR_DATA_GPIO_Port GPIOA


/*
 * Outputs
	BANCO = 0,
	CELDA_1,
	CELDA_2,
	CELDA_3,
	CELDA_4,
	CELDA_5,
	CELDA_6,
	CELDA_7

	DATA-> 1000 0001
           HGFE DCBA
*/

enum SR_PINES{
	QA,
	QB,
	QC,
	QD,
	QE,
	QF,
	QG,
	QH,
	CANT_PINES_SR
};


typedef struct {
    GPIO_TypeDef *data_port;
    uint16_t      data_pin;

    GPIO_TypeDef *clk_port;
    uint16_t      clk_pin;

    GPIO_TypeDef *latch_port;
    uint16_t      latch_pin;

    uint8_t       state;   // estado actual de las 8 salidas
} shift_register_t;


void SR_Init(shift_register_t *sr,GPIO_TypeDef *data_port,uint16_t data_pin,GPIO_TypeDef *clock_port,
		uint16_t clock_pin,GPIO_TypeDef *latch_port,
		uint16_t latch_pin);
/*void SR_EscribirByte(shift_register_t *sr,uint8_t data);
void SR_SetPin(shift_register_t *sr, uint8_t bit, uint8_t value);
void SR_TogglePin(shift_register_t *sr, uint8_t bit);*/


void SR_Reset(shift_register_t *sr);

uint8_t SR_GetEstadoActual(shift_register_t *sr);


/*
 * hay que tener cuidado en como mando la data, el clock y latch estan compartidos
 */

/*
 * Solo un pin a la vez, y sin clock
 */
void SR_SetData(shift_register_t *sr, uint8_t state);
void SR_PulseClock(shift_register_t *sr);
void SR_PulseLatch(shift_register_t *sr);
//void SR_WriteByte(shift_register_t *sr, uint8_t data);

#endif
