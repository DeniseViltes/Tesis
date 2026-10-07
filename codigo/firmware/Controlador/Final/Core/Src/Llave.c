/*
 * Llave.c
 *
 *  Created on: 23 sept 2026
 *      Author: ---
 */
#include "llave.h"
#include "main.h"

void Llave_Init(void){
	HAL_GPIO_WritePin(llave_de_emergencia_GPIO_Port, llave_de_emergencia_Pin, GPIO_PIN_RESET);
}


void Llave_Habilitar(){
	HAL_GPIO_WritePin(llave_de_emergencia_GPIO_Port, llave_de_emergencia_Pin, GPIO_PIN_SET);

}


void Llave_DispararFalla(){
	HAL_GPIO_WritePin(llave_de_emergencia_GPIO_Port, llave_de_emergencia_Pin, GPIO_PIN_RESET);
}

