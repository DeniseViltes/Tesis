/*
 * controlador.c
 *
 *  Created on: 10 jun 2026
 *      Author: ---
 */

#include "controlador.h"
#include "main.h"


static controlador_t ctrl;



static shift_register_t sr_bancos[CANT_BANCOS];
static mux_t mux_bancos[CANT_BANCOS];



//por ahora solo tengo el banco 1
static GPIO_TypeDef *sr_data_puertos[CANT_BANCOS] = {
	DATA_GPIO_Port
};

static uint16_t sr_data_pines[CANT_BANCOS] = {
	DATA_Pin
};




int verificar_banco(uint8_t banco);

void Controlador_init(void){



	for (uint8_t i = 0; i < CANT_BANCOS; i++){
		ctrl.bancos[i].id=i;

		SR_Init(&sr_bancos[i],sr_data_puertos[i],sr_data_pines[i],CLK_GPIO_Port, CLK_Pin,LATCH_GPIO_Port, LATCH_Pin);

		MUX_Init(&mux_bancos[i], S0_GPIO_Port, S0_Pin,S1_GPIO_Port, S1_Pin,S2_GPIO_Port,S2_Pin);

		Banco_Init(&ctrl.bancos[i],&sr_bancos[i],&mux_bancos[i],CELDAS_POR_BANCO);
		Controlador_ApagarBanco(i);
	}
	//harcodeo esto por ahora.
	ctrl.cant_bancos = 1;
	//apagar_banco

	Controlador_AplicarEstados();
}


void Controlador_Update(void){
	Controlador_AplicarEstados();
}



static void Controlador_ClockPulse(void){
	Banco_ClockPulse(&ctrl.bancos[0]);
}


static void Controlador_LatchPulse(void){
	Banco_LatchPulse(&ctrl.bancos[0]);
}

void Controlador_AplicarEstados(void){
	//Envio 1 bit a la vez de todos los sr y luego clockeo
	if (!Controlador_HayCambios()) {
	        return;
	    }

	for (uint8_t bit = 0; bit < CANT_PINES_SR; bit++){
			for (uint8_t sr = 0; sr< CANT_BANCOS; sr++){
				Banco_AplicarEstadoPin(&ctrl.bancos[sr], bit);
			}
			Controlador_ClockPulse();

	}
	Controlador_LatchPulse();
}


int Controlador_HayCambios(void)
{
    for (uint8_t i = 0; i < CANT_BANCOS; i++) {
        if (Banco_HayCambios(&ctrl.bancos[i])) {
            return 1;
        }
    }

    return 0;
}


// Control individual

void Controlador_EncenderCelda(uint8_t banco, uint8_t celda){
	if (verificar_banco(banco) == -1) return;
	Banco_EncenderCelda(&ctrl.bancos[banco],celda );
}

void Controlador_ApagarCelda(uint8_t banco, uint8_t celda){
	if (verificar_banco(banco) == -1) return;
	Banco_ApagarCelda(&ctrl.bancos[banco],celda);
}

//Control por bancos

int verificar_banco(uint8_t banco){
	if (banco > CANT_BANCOS || banco < 0 )
		return -1;
	return 1;
}

void Controlador_EncenderBanco(uint8_t banco){
	if (verificar_banco(banco) == -1) return;
	Banco_Encender(&ctrl.bancos[banco]);
}

void Controlador_ApagarBanco(uint8_t banco){
	if (verificar_banco(banco) == -1) return;

	Banco_Apagar(&ctrl.bancos[banco]);
}





