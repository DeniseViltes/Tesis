/*
 * Llave.c
 *
 *  Created on: 23 sept 2026
 *      Author: ---
 */
#include "llave.h"
#include "main.h"


/*
 * fallas que disparen la llave:
 * valores de descarga mayores a 3A
 * valores de carga mayores a 3A
 *
 */


void Llave_Init(void){
	HAL_GPIO_WritePin(llave_de_emergencia_GPIO_Port, llave_de_emergencia_Pin, GPIO_PIN_RESET);
}

void Llave_Habilitar(){

	HAL_GPIO_WritePin(llave_de_emergencia_GPIO_Port, llave_de_emergencia_Pin, GPIO_PIN_SET);
}


void Llave_DispararFalla(void){
	HAL_GPIO_WritePin(llave_de_emergencia_GPIO_Port, llave_de_emergencia_Pin, GPIO_PIN_RESET);
}
