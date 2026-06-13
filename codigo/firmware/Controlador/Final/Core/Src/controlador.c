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
	DATA1_GPIO_Port,
	DATA2_GPIO_Port,
	DATA3_GPIO_Port,
	DATA4_GPIO_Port
};

static uint16_t sr_data_pines[CANT_BANCOS] = {
	DATA1_Pin,
	DATA2_Pin,
	DATA3_Pin,
	DATA4_Pin
};


void Controlador_init(void){



	for (uint8_t i = 0; i < CANT_BANCOS; i++){
		ctrl.bancos[i].id=i;

		SR_Init(&sr_bancos[i],data_puerto[i],data_pines[i],CLK_GPIO_Port, CLK_Pin,LATCH_GPIO_Port, LATCH_Pin);

		MUX_Init(&mux_bancos[i], S0_GPIO_Port, S0_Pin,S1_GPIO_Port, S1_Pin,S2_GPIO_Port,S2_Pin);

		Banco_Init(&ctrl.bancos[i],&sr_bancos[i],&mux_bancos[i],CELDAS_POR_BANCO);
	}
	//harcodeo esto por ahora.
	crtl.cant_bancos = 1;

}


void Controlador_AplicarEstados(void){

}




// Control individual

void Controlador_EncenderCelda(uint8_t banco, uint8_t celda){
	if (verificar_banco(banco) == -1) return;
	Banco_EncenderCelda(ctrl.bancos[banco]);
}

void Controlador_ApagarCelda(uint8_t banco, uint8_t celda){
	if (verificar_banco(banco) == -1) return;
	Banco_ApagarCelda(ctrl.bancos[banco],celda);
}

//Control por bancos

int verificar_banco(uint8_t banco){
	if (banco > CANT_BANCOS || banco <= 0 )
		return -1;
	return banco - 1;
}

void Controlador_EncenderBanco(uint8_t banco){
	if (verificar_banco(banco) == -1) return;
	Banco_Encender(ctrl.bancos[banco]);
}

void Controlador_ApagarBanco(uint8_t banco){
	if (verificar_banco(banco) == -1) return;

	Banco_Apagar(ctrl.bancos[banco]);
}




