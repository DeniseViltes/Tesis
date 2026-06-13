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


}


void Controlador_AplicarEstados(void){

}



// Control individual

void Encender_Celda(){

}


//Control por bancos

int verificar_banco(int banco){
	if (banco > CANT_BANCOS || banco <= 0 )
		return -1;
	return banco - 1;
}

void Encender_Banco(int num_banco){
	uint8_t banco = verificar_banco(num_banco);
	if (banco == -1)
		return;

}


