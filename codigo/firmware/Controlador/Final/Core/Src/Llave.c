/*
 * Llave.c
 *
 *  Created on: 23 sept 2026
 *      Author: ---
 */
#include "Llave.h"
#include "main.h"

/* Modulo low level trigger: reposo HIGH, PWM de 2 kHz al disparar falla. */
#define BUZZER_FRECUENCIA_HZ 2000u
#define BUZZER_RELOJ_HZ 1000000u

static void Buzzer_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    uint32_t reloj_tim2 = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0u) {
        reloj_tim2 *= 2u;
    }

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM2_FORCE_RESET();
    __HAL_RCC_TIM2_RELEASE_RESET();
    /* Detiene el contador si el depurador pausa el procesador. */
    __HAL_DBGMCU_FREEZE_TIM2();
    __HAL_AFIO_REMAP_TIM2_PARTIAL_1();
    /* Libera PA15 de JTAG y conserva SWD en PA13/PA14. */
    __HAL_AFIO_REMAP_SWJ_NOJTAG();

    /* Configuracion directa: no requiere agregar el driver HAL TIM al proyecto. */
    TIM2->PSC = reloj_tim2 / BUZZER_RELOJ_HZ - 1u;
    TIM2->ARR = BUZZER_RELOJ_HZ / BUZZER_FRECUENCIA_HZ - 1u;
    TIM2->CCMR1 = TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1PE;
    TIM2->CCR1 = 0u;
    /* Polaridad invertida: CCR1 = 0 mantiene PA15 en HIGH (apagado). */
    TIM2->CCER = TIM_CCER_CC1E | TIM_CCER_CC1P;
    TIM2->CR1 = TIM_CR1_ARPE;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR = 0u;
    /* El contador se inicia solamente al disparar una falla. */

    gpio.Pin = BUZZER_Pin;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BUZZER_GPIO_Port, &gpio);
}

void Llave_Init(void){
	HAL_GPIO_WritePin(llave_de_emergencia_GPIO_Port, llave_de_emergencia_Pin, GPIO_PIN_RESET);
    Buzzer_Init();
}


void Llave_Habilitar(){
    TIM2->CR1 &= ~TIM_CR1_CEN;
    TIM2->CCR1 = 0u;
    TIM2->EGR = TIM_EGR_UG;
	HAL_GPIO_WritePin(llave_de_emergencia_GPIO_Port, llave_de_emergencia_Pin, GPIO_PIN_SET);

}


void Llave_DispararFalla(){
	HAL_GPIO_WritePin(llave_de_emergencia_GPIO_Port, llave_de_emergencia_Pin, GPIO_PIN_RESET);
    TIM2->CCR1 = (TIM2->ARR + 1u) / 2u;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->CR1 |= TIM_CR1_CEN;
}

