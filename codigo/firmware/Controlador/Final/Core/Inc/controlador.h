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






typedef struct {
	banco_t bancos[CANT_BANCOS];
	uint8_t cant_bancos;
//acá tendrian que ir las mediciones de los buses
}controlador_t;

void Controlador_init(void);
void Controlador_Update(void);
void Controlador_AplicarEstados(void);

void Controlador_BypassBanco(uint8_t banco);
void Controlador_ActivarCeldasBanco(uint8_t banco);


void Controlador_EncenderCelda(uint8_t banco, uint8_t celda);
void Controlador_ApagarCelda(uint8_t banco, uint8_t celda);

/*
Controlador_SetBancos(...)
Controlador_SetCeldasDeBanco(...)*/


#endif /* INC_CONTROLADOR_H_ */
