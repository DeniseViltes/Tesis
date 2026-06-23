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
#include "adc.h"


#define CELDAS_POR_BANCO 7

typedef enum {
  OFF = 0,
  ON  = 1
} SW_estado_t;

typedef enum {
  FIJO = 0,
  SYNCHRO,
  COMPLEMENTARY
} celda_modo_t;


typedef struct{
	uint8_t id;
	uint8_t pin_mux;
	uint8_t pin_sr;
	SW_estado_t actual;
	SW_estado_t prox;
	celda_modo_t modo;
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
	uint16_t contador;
	uint16_t periodo_ms;  //si es cero no switchea.
	uint8_t fase; //fase general del banco
	adc_node_t canal_adc;
} banco_t;


/*
 * Configura los parametros principales del banco, e inicia el banco apagado (sw banco encendido)
 */
void Banco_Init(banco_t *banco, shift_register_t *sr, mux_t *mux, uint8_t cant_celdas, adc_node_t nodo);

/*
 * Enciende el switch del banco para mantener la continuidad de la matriz
 */
void Banco_EncenderSwitch(banco_t *banco);

/*
 * Apaga el switch del banco, para permitir encender una celda
 */
void Banco_ApagarSwitch(banco_t *banco);

/*
 * Apaga todas las celdas, recordar que luego hay que activar el switch del banco
 */
void Banco_ApagarCeldas(banco_t *banco);

/*
 * Enciende todas las celdas del banco, recordar apagar el switch del banco primero
 */
void Banco_EncenderCeldas(banco_t *banco);

/*
 * Comanda el proximo estado de la celda, sin cambiarla  y pone el modo en simple (sin switching)
 */
void Banco_EncenderCelda(banco_t *banco, uint8_t celda);

/*
 * Cambia a apagado una celda y pone el modo en simple (sin switching)
 */
void Banco_ApagarCelda(banco_t *banco, uint8_t celda);

void Banco_AplicarEstadoPin(banco_t *banco, uint8_t pin);


void Banco_ConfigCelda(banco_t *banco,uint8_t celda,uint8_t pin_sr,uint8_t pin_mux);

void Banco_ClockPulse(banco_t *banco);

void Banco_LatchPulse(banco_t *banco);

int Banco_HayCeldasProxON(banco_t *banco);
int Banco_HayCeldasActualON(banco_t *banco);

int Banco_HayCambios(banco_t *banco);

void Banco_SetModoCelda(banco_t *banco, uint8_t celda, celda_modo_t modo);

void Banco_SwichingCelda(banco_t *banco, uint8_t celda);

void Banco_ModificarFase(banco_t *banco, uint8_t fase);

void Banco_ModificarPeriodo(banco_t *banco, uint16_t periodo_ms);

//void Banco_CalcularSwitching(banco_t *banco);

SW_estado_t  Banco_GetEstado (banco_t *banco);
SW_estado_t Banco_GetEstadoCelda(banco_t *banco, uint8_t pin);
SW_estado_t  Banco_GetEstadoActual (banco_t *banco);

SW_estado_t Banco_GetEstadoPin(banco_t *banco, uint8_t celda);
uint8_t Banco_CalcularPatron(banco_t *banco);
void Banco_SetPatronCeldas(banco_t *banco, uint8_t patron);
void Banco_Tick(banco_t *banco);


/*
 * Detiene el switching y apaga la celda
 */
void Banco_DetenerSwitchingCelda(banco_t *banco, uint8_t celda);


#endif /* INC_BANCO_H_ */
