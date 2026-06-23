// shift_register.c

#include "shift_register.h"



// --------------------------------------------------
// Pulso de clock
// --------------------------------------------------
void SR_PulseClock(shift_register_t *sr)
{
    // DATA ya fue seteado antes de llamar a esta función
	if (sr == NULL ) {
			return;
		}
    HAL_Delay(1); // tiempo para que DATA quede estable

    HAL_GPIO_WritePin(sr->clk_port, sr->clk_pin, GPIO_PIN_SET);

    HAL_Delay(1);

    HAL_GPIO_WritePin(sr->clk_port, sr->clk_pin, GPIO_PIN_RESET);
    HAL_Delay(1); //lo puse ahora

}


// --------------------------------------------------
// Pulso de latch
// --------------------------------------------------

void SR_PulseLatch(shift_register_t *sr){

	if (sr == NULL ) {
			return;
		}
    HAL_Delay(1);

    HAL_GPIO_WritePin(sr->latch_port, sr->latch_pin, GPIO_PIN_SET);

    HAL_Delay(1);

    HAL_GPIO_WritePin(sr->latch_port, sr->latch_pin, GPIO_PIN_RESET);
}

// --------------------------------------------------
// Inicialización
// --------------------------------------------------

void SR_config(shift_register_t *sr,GPIO_TypeDef *data_port,uint16_t data_pin,GPIO_TypeDef *clock_port,
		uint16_t clock_pin,GPIO_TypeDef *latch_port,
		uint16_t latch_pin){

}

void SR_Init(shift_register_t *sr,GPIO_TypeDef *data_port,uint16_t data_pin,GPIO_TypeDef *clock_port,
		uint16_t clock_pin,GPIO_TypeDef *latch_port,
		uint16_t latch_pin){
	sr->state = 0x00;
	sr->data_pin=data_pin;
	sr->data_port=data_port;
	sr->clk_pin=clock_pin;
	sr->clk_port=clock_port;
	sr->latch_pin= latch_pin;
	sr->latch_port = latch_port;

    HAL_GPIO_WritePin(sr->data_port, sr->data_pin,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(sr->clk_port, sr->clk_pin,   GPIO_PIN_RESET);
    HAL_GPIO_WritePin(sr->latch_port, sr->latch_pin, GPIO_PIN_RESET);

}


void SR_SetData(shift_register_t *sr, uint8_t bit){

	if (sr == NULL ) {
			return;
		}
	HAL_GPIO_WritePin(sr->data_port, sr->data_pin, bit ? GPIO_PIN_SET : GPIO_PIN_RESET);
}





// --------------------------------------------------
// Escribir 8 bits al 74HC595
// --------------------------------------------------
/*
void SR_EscribirByte(shift_register_t *sr, uint8_t data)
{
	sr->state = data;
    HAL_GPIO_WritePin(sr->latch_port, sr->latch_pin, GPIO_PIN_RESET);

    for (int i = 7; i >= 0; i--)
    {
        GPIO_PinState bit =
            (data & (1 << i)) ? GPIO_PIN_SET : GPIO_PIN_RESET;

        HAL_GPIO_WritePin(sr->data_port, sr->data_pin, bit);

        SR_Clock(sr);
    }

    HAL_GPIO_WritePin(sr->data_port, sr->data_pin, GPIO_PIN_RESET);

    HAL_Delay(1);

    HAL_GPIO_WritePin(sr->latch_port, sr->latch_pin, GPIO_PIN_SET);
    HAL_Delay(1);

    HAL_GPIO_WritePin(sr->latch_port, sr->latch_pin, GPIO_PIN_RESET);
    HAL_Delay(1);
}


void SR_ApagarPin(shift_register_t *sr, uint8_t bit)
{
	sr->state &= ~(1U << bit);
	SR_EscribirByte(sr, sr->state);
}

void SR_PrenderPin(shift_register_t *sr, uint8_t bit)
{
    sr->state |= (1U << bit);
    SR_EscribirByte(sr, sr->state);
}


uint8_t SR_GetEstadoActual(shift_register_t *sr){
	return sr->state;
}


void SR_TogglePin(shift_register_t *sr, uint8_t pin){
	sr->state ^= (1U << pin);
	SR_EscribirByte(sr, sr->state);
}

void SR_Reset(shift_register_t *sr){
	SR_EscribirByte(sr, 0x00);
}*/
