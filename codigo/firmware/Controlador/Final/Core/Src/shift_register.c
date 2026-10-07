#include "shift_register.h"



void SR_PulseClock(shift_register_t *sr)
{
    if (sr == NULL) {
        return;
    }

    sr->clk_port->BSRR = sr->clk_pin;                 // CLK = 1

    sr->clk_port->BSRR = (uint32_t)sr->clk_pin << 16; // CLK = 0
}




void SR_PulseLatch(shift_register_t *sr)
{
    if (sr == NULL) {
        return;
    }

    sr->latch_port->BSRR = sr->latch_pin;                 // latch = 1
   //HAL_Delay(1);
    sr->latch_port->BSRR = (uint32_t)sr->latch_pin << 16; // latch = 0
}

// --------------------------------------------------
// Inicialización
// --------------------------------------------------


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

	sr->data_port->BSRR  = (uint32_t)sr->data_pin  << 16U;
	sr->clk_port->BSRR   = (uint32_t)sr->clk_pin   << 16U;
	sr->latch_port->BSRR = (uint32_t)sr->latch_pin << 16U;

}


void SR_SetData(shift_register_t *sr, uint8_t bit){

	if (sr == NULL ) {
			return;
		}
	sr->data_port->BSRR = (uint32_t)sr->data_pin << (bit ? 0U : 16U);
}


