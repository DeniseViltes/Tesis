/*
 * banco.h
 *
 *  Created on: 11 jun 2026
 *      Author: ---
 */

#ifndef INC_BANCO_H_
#define INC_BANCO_H_

#include "shift_register.h"
#include "mux.h"


#define CELDAS_POR_BANCO 3

typedef enum {
  OFF = 0,
  ON  = 1
} SW_estado_t;

typedef struct{
	uint8_t id;
	uint8_t pin_mux;
	uint8_t pin_sr;
	SW_estado_t actual; // necesito?
	SW_estado_t prox;
}celda_t;

typedef struct {
	uint8_t id;
	shift_register_t *sr;
	mux_t *mux;
	uint8_t cant_celdas;
	celda_t celdas[CELDAS_POR_BANCO];
	SW_estado_t actual;
	SW_estado_t prox;
	uint8_t pin_sr;
} banco_t;



/*
 * Configura los parametros principales del banco, e inicia el banco apagado (sw banco encendido)
 */
void Banco_Init(banco_t *banco, shift_register_t *sr, mux_t *mux, uint8_t cant_celdas);

/*
 * Enciende todas las celdas del banco
 */
void Banco_Encender(banco_t *banco);

/*
 * Apaga todas las celdas del banco
 */
void Banco_Apagar(banco_t *banco);


/*
 * Comanda el proximo estado de la celda, sin cambiarla
 */
void Banco_EncenderCelda(banco_t *banco, uint8_t celda);
void Banco_ApagarCelda(banco_t *banco, uint8_t celda);

void Banco_AplicarEstadoPin(banco_t *banco, uint8_t pin);

void Banco_ConfigCelda(banco_t *banco,uint8_t celda,uint8_t pin_sr,uint8_t pin_mux);

void Banco_ClockPulse(banco_t *banco);

void Banco_LatchPulse(banco_t *banco);


SW_estado_t  Banco_GetEstado (banco_t *banco);

#endif /* INC_BANCO_H_ */
