/* Project includes. */
#include <adc.h>
#include "main.h"


/* App includes. */


/* Application includes. */


/********************** macros and definitions *******************************/
typedef struct
{
    uint32_t r1;
    uint32_t r2;
} adc_divisor_t;

#define ADC_VREF_mV 3300u
#define ADC_SETS   16
#define ADC_BUF_LEN (ADC_NODE_COUNT * ADC_SETS)


/********************** external data declaration *****************************/

extern ADC_HandleTypeDef hadc1;


/********************** external functions definition ************************/



/********************** internal data declaration ****************************/

volatile uint16_t g_adc_raw[ADC_NODE_COUNT];
static uint8_t g_adc_dma_started = 0u;

volatile uint16_t adc_dma_buf[ADC_BUF_LEN];

/********************** internal data definition *****************************/


/*
 * Divisor de cada canal:
 *
 * Vmeas ---- R1 ----+---- R2 ---- GND
 *                   |
 *                 ADC pin
 *
 * Vadc = Vmeas * R1 / (R1 + R2)
 * Vmeas  = Vadc * (R1 + R2) / R1
 *
 *
 * Rank 1  -> ADC_NODE_CELL_11
 * Rank 2  -> ADC_NODE_CELL_12
 * Rank 3  -> ADC_NODE_CELL_21
 * Rank 4  -> ADC_NODE_CELL_22
 * Rank 5  -> ADC_NODE_CELL_31
 * Rank 6  -> ADC_NODE_CELL_32
 * Rank 7  -> ADC_NODE_OUT_POS
 * Rank 8  -> ADC_NODE_OUT_NEG
 * Rank 9  -> ADC_NODE_BUS_1
 * Rank 10 -> ADC_NODE_BUS_2
 */
static const adc_divisor_t adc_divisores[ADC_NODE_COUNT] =
{
    [ADC_NODE_CELL_11] = { .r1 = 10000u, .r2 = 33000u },
    [ADC_NODE_CELL_12] = { .r1 = 10000u, .r2 = 33000u },
    [ADC_NODE_CELL_21] = { .r1 = 10000u, .r2 = 33000u },
    [ADC_NODE_CELL_22] = { .r1 = 10000u, .r2 = 33000u },
    [ADC_NODE_CELL_31] = { .r1 = 10000u, .r2 = 33000u },
    [ADC_NODE_CELL_32] = { .r1 = 10000u, .r2 = 33000u },
    [ADC_NODE_OUT_POS] = { .r1 = 10000u, .r2 = 33000u },
    [ADC_NODE_OUT_NEG] = { .r1 = 10000u, .r2 = 33000u },
    [ADC_NODE_BUS_1]   = { .r1 = 10000u, .r2 = 33000u },
    [ADC_NODE_BUS_2]   = { .r1 = 10000u, .r2 = 33000u }
};


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

float adc_to_voltage(uint16_t raw)
{
    return (3.3f * raw) / 4095.0f;
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


uint16_t adc_get_pin_voltage(adc_node_t node)
{
    uint16_t raw;

    if ((uint32_t)node >= ADC_NODE_COUNT)
    {
        return 0u;
    }

    raw = adc_get_raw(node);

    return (uint16_t)(((uint32_t)raw * ADC_VREF_mV) / 4095u);
}


uint16_t adc_get_meas_voltage(adc_node_t node)
{
    uint32_t vadc;
    uint32_t r1;
    uint32_t r2;

    if ((uint32_t)node >= ADC_NODE_COUNT)
    {
        return 0u;
    }

    vadc = adc_get_pin_voltage(node);
    r1 = adc_divisores[(uint32_t)node].r1;
    r2 = adc_divisores[(uint32_t)node].r2;

    if (r2 == 0u)
    {
        return 0u;
    }

    return (uint16_t)((vadc * (r1 + r2)) / r2);
}
