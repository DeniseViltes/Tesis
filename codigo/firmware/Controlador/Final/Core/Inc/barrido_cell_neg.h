/*
 * barrido_call_neg.h
 *
 *  Created on: 26 oct 2026
 *      Author: ---
 */

#ifndef INC_BARRIDO_CELL_NEG_H_
#define INC_BARRIDO_CELL_NEG_H_

#include "banco.h"

#define INTERVALO_MEDICION 1000u

/* Valor inicial a validar con el circuito analogico. */
#define BARRIDO_CELL_NEG_ASENTAMIENTO_MS 0u

void BarridoCellNeg_Init(banco_t *bancos, uint8_t cantidad);

void BarridoCellNeg_Update(void);

uint8_t BarridoCellNeg_Ocupado(void);

/* Indices desde cero. Devuelve 1 si hay un barrido completo disponible.
 * No modifica *valor si no hay datos o los argumentos son invalidos. */
int BarridoCellNeg_GetMedicion(uint8_t banco, uint8_t celda, uint16_t *valor);
uint32_t BarridoCellNeg_GetActualizacionMs(void);



#endif /* INC_BARRIDO_CELL_NEG_H_ */
