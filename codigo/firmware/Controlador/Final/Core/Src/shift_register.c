// shift_register.c

#include "shift_register.h"

// --------------------------------------------------
// Pulso de clock
// --------------------------------------------------
static void SR_ClockPulse(void)
{
    // DATA ya fue seteado antes de llamar a esta función

    HAL_Delay(1); // tiempo para que DATA quede estable

    HAL_GPIO_WritePin(CLK_GPIO_Port, CLK_Pin, GPIO_PIN_SET);
    HAL_Delay(1);

    HAL_GPIO_WritePin(CLK_GPIO_Port, CLK_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
}
// --------------------------------------------------
// Inicialización
// --------------------------------------------------
void SR_Init(void)
{
    HAL_GPIO_WritePin(DATA_GPIO_Port,  DATA_Pin,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(CLK_GPIO_Port,   CLK_Pin,   GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LATCH_GPIO_Port, LATCH_Pin, GPIO_PIN_RESET);
}

// --------------------------------------------------
// Escribir 8 bits al 74HC595
// --------------------------------------------------
void SR_WriteByte(uint8_t data)
{
    HAL_GPIO_WritePin(LATCH_GPIO_Port, LATCH_Pin, GPIO_PIN_RESET);

    for (int i = 7; i >= 0; i--)
    {
        GPIO_PinState bit =
            (data & (1 << i)) ? GPIO_PIN_SET : GPIO_PIN_RESET;

        HAL_GPIO_WritePin(DATA_GPIO_Port, DATA_Pin, bit);

        SR_ClockPulse();
    }

    HAL_GPIO_WritePin(DATA_GPIO_Port, DATA_Pin, GPIO_PIN_RESET);

    HAL_Delay(1);

    HAL_GPIO_WritePin(LATCH_GPIO_Port, LATCH_Pin, GPIO_PIN_SET);
    HAL_Delay(1);

    HAL_GPIO_WritePin(LATCH_GPIO_Port, LATCH_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
}
