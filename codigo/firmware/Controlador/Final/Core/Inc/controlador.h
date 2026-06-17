/*
 * controlador.h
 *
 *  Created on: 10 jun 2026
 *      Author: ---
 */

#ifndef INC_CONTROLADOR_H_
#define INC_CONTROLADOR_H_


#include "banco.h"
#include "shift_register.h"
#include "mux.h"


#define CANT_BANCOS 1
#define FREQ_DEFAULT 50


typedef struct {
	banco_t bancos[CANT_BANCOS];
	uint8_t cant_bancos;
//acá tendrian que ir las mediciones de los buses
}controlador_t;

void Controlador_init(void);
void Controlador_Update(void);
void Controlador_AplicarEstados(void);
uint8_t Controlador_GetDataSR(uint8_t banco);

void Controlador_BypassBanco(uint8_t banco);
void Controlador_ActivarCeldasBanco(uint8_t banco);

void Controlador_Reiniciar(void);
uint8_t Controlador_GetEstadoCelda(uint8_t banco, uint8_t celda);
uint8_t Controlador_GetEstadoBanco(uint8_t banco);

void Controlador_EncenderCelda(uint8_t banco, uint8_t celda);
void Controlador_ApagarCelda(uint8_t banco, uint8_t celda);
void Controlador_ActualizarSwitching(void);

int Controlador_HayCambios(void);

void Controlador_Tick1ms(void);

void Controlador_IniciarSwitchingCelda(uint8_t banco, uint8_t celda, char modo);

void Controlador_ActualizarEstados(void);

void Controlador_ModificarPeriodoBanco(uint8_t banco, uint16_t periodo_ms);


void Controlador_DetenerSwitchingBancoBypass(uint8_t banco);
/*
Controlador_SetBancos(...)
Controlador_SetCeldasDeBanco(...)*/


#endif /* INC_CONTROLADOR_H_ */
