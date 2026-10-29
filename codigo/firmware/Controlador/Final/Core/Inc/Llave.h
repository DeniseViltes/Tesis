/*
 * Llave.h
 *
 *  Created on: 23 sept 2026
 *      Author: ---
 */

#ifndef INC_LLAVE_H_
#define INC_LLAVE_H_

//Inicializa la llave en LOW y el buzzer de PA15 apagado (reserva TIM2).
void Llave_Init(void);


//Sube la llave a HIGH y apaga el buzzer.
void Llave_Habilitar(void);

//Deshabilita la llave, la baja a LOW y activa el buzzer a 2 kHz.
void Llave_DispararFalla(void);


#endif /* INC_LLAVE_H_ */
