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




int Banco_HayCeldasON(banco_t *banco);
int Banco_HayCambios(banco_t *banco);
static void Banco_SetCelda(banco_t *banco, uint8_t celda, SW_estado_t estado);
static SW_estado_t calcular_estado_celda(celda_modo_t modo, uint8_t fase);


/*
 * 0 : CELL_NEG_7 : c = 6
 * 1 : CELL_NEG_6 : c = 5
 * 2 : CELL_NEG_5 : c = 4
 * 3 : GND
 * 4 : CELL_NEG_2 : c = 1
 * 5 : CELL_NEG_4 : c = 3
 * 6 : CELL_NEG_1 : c = 0
 * 7 : CELL_NEG_3 : c = 2
 */

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
	default: return 3;
	}
}

void Banco_Init(banco_t *banco, shift_register_t *sr, mux_t *mux, uint8_t cant_celdas){
	banco->sr = sr;
	banco->mux = mux;
	banco->cant_celdas = cant_celdas;
	banco->pin_sr = 0;
	banco->contador = 0;
	banco->frecuencia = 0; //Cero significa constante
	banco->fase = 0;  //fase de referencia para la celda.


	for(int c = 0 ; c< cant_celdas; c++){
		Banco_ConfigCelda(banco,c, c+1, Controlador_GetMuxPinCelda(c));

		banco->celdas[c].modo = FIJO;


	}
}





void Banco_ConfigCelda(banco_t *banco,uint8_t celda,uint8_t pin_sr,uint8_t pin_mux){
	banco->celdas[celda].pin_sr = pin_sr;
	banco->celdas[celda].pin_mux = pin_mux;
}





void Banco_EncenderSwitch(banco_t *banco){
	if (banco == NULL) {
	        return;
	    }
	banco->prox=ON;
}

void Banco_ApagarSwitch(banco_t *banco){
	if (banco == NULL) {
	        return;
	    }
	banco->prox=OFF;
}

void Banco_ApagarCeldas(banco_t *banco){
    for (uint8_t i = 0; i < banco->cant_celdas; i++) {
    	banco->celdas[i].prox = OFF;
    	banco->celdas[i].modo = FIJO;
    }

}


void Banco_EncenderCeldas(banco_t *banco){
    for (uint8_t i = 0; i < banco->cant_celdas; i++) {
    	banco->celdas[i].prox = ON;
    	banco->celdas[i].modo = FIJO;
    }

}

void Banco_EncenderCelda(banco_t *banco, uint8_t celda){
	Banco_SetCelda(banco, celda, ON);
	banco->prox=OFF;
	banco->celdas[celda].modo = FIJO;
}
/*
 * Pone el estado proxde la celda en OFF
 */
void Banco_ApagarCelda(banco_t *banco, uint8_t celda){
	Banco_SetCelda(banco, celda, OFF);
	if (!Banco_HayCeldasON(banco)){
		banco->prox = ON;
	}
	banco->celdas[celda].modo =FIJO;
}


static void Banco_SetCelda(banco_t *banco, uint8_t celda, SW_estado_t estado){
    if (banco == NULL) {
        return;
    }

    if (celda >= banco->cant_celdas) {
        return;
    }

	banco->celdas[celda].prox = estado;
}


SW_estado_t  Banco_GetEstado (banco_t *banco){
	return banco->actual;
}

int Banco_HayCambios(banco_t *banco){
	int cambios = 0;
	cambios += (banco->actual!= banco->prox)? 1: 0;

	for (uint8_t i = 0;i< banco->cant_celdas;i++){
		cambios += (banco->celdas[i].actual!= banco->celdas[i].prox)? 1: 0;
	}
	return cambios;
}


static void Banco_AplicarEstadosBanco(banco_t *banco){

	SR_SetData(banco->sr, banco->prox);
	banco->actual = banco->prox;
}


static void Banco_AplicarEstadosCeldas(banco_t *banco, uint8_t celda){
	if (celda >= banco->cant_celdas){
		SR_SetData(banco->sr, OFF);
	}
	else{
		SR_SetData(banco->sr, banco->celdas[celda].prox);
		banco->celdas[celda].actual = banco->celdas[celda].prox;
	}
}



void Banco_AplicarEstadoPin(banco_t *banco, uint8_t pin){
	if(pin == 0){
		Banco_AplicarEstadosBanco(banco);
	}
	else {
		Banco_AplicarEstadosCeldas(banco, pin - 1);
	}
	return;
}

int Banco_HayCeldasON(banco_t *banco)
{
    for (uint8_t c = 0; c < banco->cant_celdas; c++) {
        if (banco->celdas[c].prox == ON) {
            return 1;
        }
    }
    return 0;
}




void Banco_ClockPulse(banco_t *banco){
	SR_PulseClock(banco->sr);
}


void Banco_LatchPulse(banco_t *banco){
	SR_PulseLatch(banco->sr);
}


//------------------------------SWITCHING---------------------------------------


void Banco_SetModoCelda(banco_t *banco, uint8_t celda, celda_modo_t modo)
{

	if (celda >= banco->cant_celdas) {
		return;
	}

	banco->celdas[celda].modo = modo;
}



static SW_estado_t Banco_CalcularEstadoSwitching(celda_modo_t modo, uint8_t fase){

	switch (modo) {
	    case SYNCHRO:
	    	return fase ? OFF : ON;
	    case COMPLEMENTARY:
	    	return fase ? ON : OFF;
	    default:
	    	return OFF;
}



void Banco_ActualizarSwitching(banco_t *banco){

	if (!banco->frecuencia) return;

	for (uint8_t c = 0; c < banco->cant_celdas; c++){
		banco->celdas[c].prox = Banco_CalcularEstadoSwitching(banco->celdas[c].modo, banco->fase);
	}
}


void Banco_ModificarFase(banco_t *banco, uint8_t fase){
	banco->fase = fase ? 1 : 0;
}
