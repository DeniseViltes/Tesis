// shift_register.h

#ifndef SHIFT_REGISTER_H
#define SHIFT_REGISTER_H

#include "main.h"
#include <stdint.h>

// Pines definidos en CubeMX/main.h
// Ejemplo:
// #define SR_DATA_Pin GPIO_PIN_0
// #define SR_DATA_GPIO_Port GPIOA

void SR_Init(void);
void SR_WriteByte(uint8_t data);

#endif
