/*
 * adc.c
 *
 *  Created on: 21 jun 2026
 *      Author: ---
 */
#include <adc.h>
#include "main.h"

#define ADC_VREF_mV 3300u
#define ADC_SETS   128u
#define ADC_BUF_LEN (ADC_NODE_COUNT * ADC_SETS)

extern ADC_HandleTypeDef hadc1;


volatile uint16_t g_adc_raw[ADC_NODE_COUNT];
static uint8_t g_adc_dma_started = 0u;

volatile uint16_t adc_dma_buf[ADC_BUF_LEN];

/********************** internal functions definitions ***********************/
volatile uint8_t adc_half_ready = 0;
volatile uint8_t adc_full_ready = 0;

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc);

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc);

static void adc_process_block(volatile uint16_t *p, uint16_t len);

/********************** internal functions declaration ***********************/

void adc_init(void)
{
    if (g_adc_dma_started != 0u)
    {
        return;
    }

    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_dma_buf, ADC_BUF_LEN) == HAL_OK)
    {
        g_adc_dma_started = 1u;
    }
}



void adc_update(void)
{
    if (adc_half_ready)
    {
        adc_half_ready = 0;
        adc_process_block((uint16_t*)&adc_dma_buf[0], ADC_BUF_LEN / 2);
    }

    if (adc_full_ready)
    {
        adc_full_ready = 0;
        adc_process_block((uint16_t*)&adc_dma_buf[ADC_BUF_LEN / 2], ADC_BUF_LEN / 2);
    }
}


uint16_t adc_get_raw(adc_node_t node)
{
    if ((uint32_t)node >= ADC_NODE_COUNT)
    {
        return 0u;
    }

    return g_adc_raw[(uint32_t)node];
}



static void adc_process_block(volatile uint16_t *p, uint16_t len)
{
	if ((p == NULL) || (len == 0u) || ((len % ADC_NODE_COUNT) != 0u))
	{
	    return;
	}
	uint32_t g_adc_accum[ADC_NODE_COUNT]={0};

    uint16_t samples = 0;


    for (uint16_t i = 0; i < len; i += ADC_NODE_COUNT)
    {
        for (int k = 0; k < ADC_NODE_COUNT; k++)
        {
            g_adc_accum[k] += p[i + k];
        }
        samples++;
    }

    if (samples == 0u)
    {
        return;
    }


    for (int ch = 0; ch < ADC_NODE_COUNT; ch++)
    {
        g_adc_raw[ch] = (uint16_t)(g_adc_accum[ch] / samples);
    }
}


uint8_t adc_is_dma_started(void)
{
  return g_adc_dma_started;
}


void adc_get_buffer(uint16_t *buffer, uint16_t len)
{
    if ((buffer == 0) || (len < ADC_NODE_COUNT))
    {
        return;
    }

    for (uint8_t i = 0; i < len; i++)
    {
        buffer[i] = (uint16_t)(((uint32_t)g_adc_raw[i] * ADC_VREF_mV) / 4095u);
    }
}




void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
        adc_half_ready = 1;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
        adc_full_ready = 1;
}





