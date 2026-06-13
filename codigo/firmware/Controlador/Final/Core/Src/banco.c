/*
 * banco.c
 *
 *  Created on: 10 jun 2026
 *      Author: ---
 */
#include "banco.h"


/*
 * Outputs
A	BANCO = 0,
B	CELDA_1,
C	CELDA_2,
D	CELDA_3,
E	CELDA_4,
F	CELDA_5,
G	CELDA_6,
H	CELDA_7

	DATA-> 1000 0001
           HGFE DCBA
*/

#define BANCO_OFF 0x01
#define BANCO_ON 0xFE
#define RESET 0x00

static uint8_t Controlador_GetMuxPinCelda(uint8_t celda)
{
	switch (celda) {
	case 0: return 6; // CELL_NEG_1
	case 1: return 4; // CELL_NEG_2
	case 2: return 7; // CELL_NEG_3
	case 3: return 5; // CELL_NEG_4
	case 4: return 2; // CELL_NEG_5
	case 5: return 1; // CELL_NEG_6
	case 6: return 0; // CELL_NEG_7
	default: return 0;
	}
}

void Banco_Init(banco_t *banco, shift_register_t *sr, mux_t *mux, uint8_t cant_celdas){
	banco->sr = sr;
	banco->mux = mux;
	banco->cant_celdas = cant_celdas;
	banco->pin_sr = 0;


	for(int c = 0 ; c< cant_celdas; c++){
		Banco_ConfigCelda(banco,c, c+1, Controlador_GetMuxPinCelda(c));
	}

	Banco_Apagar(banco);
	Banco_AplicarEstados(banco);
}

void Banco_encender(banco_t *banco){
	if (banco == NULL) {
	        return;
	    }
	for (uint8_t i = 0; i < banco->cant_celdas; i++){
			banco->celdas[i].prox = ON;
			banco->celdas[i].actual = ON;
		}
	banco->actual=OFF;
	SR_EscribirByte(banco->sr, BANCO_ON);
}

void Banco_apagar(banco_t *banco){
	if (banco == NULL) {
	        return;
	    }
	for (uint8_t i = 0; i < banco->cant_celdas; i++){
		banco->celdas[i].prox = OFF;
		banco->celdas[i].actual = OFF;
	}
	banco->actual=ON;
	SR_EscribirByte(banco->sr, BANCO_OFF);
}

void Banco_SetCelda(banco_t *banco, uint8_t celda, SW_estado_t estado){
    if (banco == NULL) {
        return;
    }

    if (celda >= banco->cant_celdas) {
        return;
    }

	banco->celdas[celda].prox = estado;
}




static int HayCambios(const banco_t *banco, uint8_t *celdas_encendidas);
static void ArmarBit(uint8_t *data, uint8_t bit, uint8_t value);
static uint8_t Actualizar_Data(banco_t *banco);





void Banco_AplicarEstados(banco_t *banco){
	if (banco == NULL) {
	        return;
	    }

	uint8_t cant_celdas_encendidas = 0;
	uint8_t cambios = HayCambios(banco, &cant_celdas_encendidas);
	if (cambios == 1){
		if (cant_celdas_encendidas > 0 && banco->actual == ON){
			SR_EscribirByte(banco->sr,RESET);
		}
		if (cant_celdas_encendidas == 0 && banco->actual == OFF){
					SR_EscribirByte(banco->sr, BANCO_OFF); //apago el banco, y prendo el sw del banco
					return;
				}

	}
	uint8_t data = Actualizar_Data(banco);
	SR_EscribirByte(banco->sr, data);

}


void Banco_ConfigCelda(banco_t *banco,uint8_t celda,uint8_t pin_sr,uint8_t pin_mux){
	banco->celdas[celda].pin_sr = pin_sr;
	banco->celdas[celda].pin_mux = pin_mux;
}

/*
 * Casos:
 *  Banco     Celda A    Celda B   ----->
 *    0          0          0
 *    1          0          0
 *    0          1          0
 *    0          0          1
 *    0          1          1
 *
 *
 *    Condiciones:
 *     0  0 0   -- >  1  0 0
 *     1  0 1   -->   0  0 1  || 1  1 0   -->   1  1 0   ||1  1 1   -->   0  1 1
 *     0
 *
 *
 *    Estado inicial? -> prendo el banco
 *    Prender una celda? Apago el banco y prendo la celda
 *    Apagar una celda? Apago la celda y prendo el banco
 *
 */


/*
 * Si hay cambios-> 1, si no hay cambios -> 0
 */
static int HayCambios(const banco_t *banco, uint8_t *celdas_encendidas)
{
	uint8_t cambios = 0;

    for (uint8_t i = 0; i < banco->cant_celdas; i++) {
        if (banco->celdas[i].prox != banco->celdas[i].actual) {
        	cambios = 1;
        }
        if (banco->celdas[i].prox == ON){
                		*celdas_encendidas +=1;
                	}
    }

    return cambios;
}

static void ArmarBit(uint8_t *data, uint8_t bit, uint8_t value)
{
    if (value) {
        *data |= (1u << bit);
    } else {
        *data &= ~(1u << bit);
    }
}

/*
 * el bit más a la izquierda  = bit 7
 * el bit más a la derecha    = bit 0
 */

/*
 * Arma el vector de data y actualiza los estados de las celdas
 */
static uint8_t Actualizar_Data(banco_t *banco){
	//parto de tener el pin banco -> 0, o prendo el sw banco, o prendo los sw celdas, ambos no es posible.

	 uint8_t data = 0x00;
	 ArmarBit(&data, banco->pin_sr , 0);

	 for (uint8_t i = 0; i < banco->cant_celdas; i++){
		 ArmarBit(&data, banco->celdas[i].pin_sr ,banco->celdas[i].prox);
		 banco->celdas[i].actual = banco->celdas[i].prox;
	 }
	 return data;
}
