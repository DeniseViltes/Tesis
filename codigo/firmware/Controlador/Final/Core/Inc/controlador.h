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
#define CELDAS_POR_BANCO 3





typedef struct {
	banco_t bancos[CANT_BANCOS];
	uint8_t cant_bancos;
//acá tendrian que ir las mediciones de los buses
}controlador_t;

void Controlador_init(void);
void Controlador_AplicarEstados(void);
#endif /* INC_CONTROLADOR_H_ */
