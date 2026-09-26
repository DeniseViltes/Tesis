/*
 * Llave.h
 *
 *  Created on: 23 sept 2026
 *      Author: ---
 */

#ifndef INC_LLAVE_H_
#define INC_LLAVE_H_

//Inicializa la llave en LOW
void Llave_Init(void);


//Sube la llave a HIGH
void Llave_Habilitar(void);

//Deshabilita la llave, la baja a LOW
void Llave_DispararFalla(void);

//Para habilitar
//Llave_Rearmar();
#endif /* INC_LLAVE_H_ */
