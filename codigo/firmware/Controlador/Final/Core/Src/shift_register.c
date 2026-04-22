#include "shift_register.h"

typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
} sr_pin_t;

static sr_pin_t sr_data  = {DATA_GPIO_Port, DATA_Pin};
static sr_pin_t sr_clk   = {CLOK_GPIO_Port, CLOK_Pin};
static sr_pin_t sr_latch = {LATCH_GPIO_Port, LATCH_Pin};



static void ShiftReg_ClockPulse(void)
{
    HAL_GPIO_WritePin(sr_clk.port, sr_clk.pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(sr_clk.port, sr_clk.pin, GPIO_PIN_RESET);
}

void ShiftReg_Init(void)
{
    HAL_GPIO_WritePin(sr_data.port, sr_data.pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(sr_clk.port, sr_clk.pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(sr_latch.port, sr_latch.pin, GPIO_PIN_RESET);
}

void ShiftReg_WriteByte(uint8_t data)
{
    HAL_GPIO_WritePin(sr_latch.port, sr_latch.pin, GPIO_PIN_RESET);

    for (int i = 7; i >= 0; i--)
    {
        HAL_GPIO_WritePin(
        		sr_data.port,
				sr_data.pin,
            (data & (1 << i)) ? GPIO_PIN_SET : GPIO_PIN_RESET
        );

        ShiftReg_ClockPulse();
    }

    HAL_GPIO_WritePin(sr_latch.port, sr_latch.pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(sr_latch.port, sr_latch.pin, GPIO_PIN_RESET);
}
