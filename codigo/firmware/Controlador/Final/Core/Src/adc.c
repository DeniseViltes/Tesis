/*
 * adc.c
 *
 *  Created on: 21 jun 2026
 *      Author: ---
 */
#include <adc.h>
#include "main.h"

#define ADC_VREF_mV 3300u
#define ADC_SETS   6u
#define ADC_BUF_LEN (ADC_NODE_COUNT * ADC_SETS)


#define DIV_R1_OHM 33000u
#define DIV_R2_OHM  8200u
#define BANCO_R_ENTRADA_OHM 39000u
#define BANCO_R_REALIMENTACION_OHM 33000u
#define INA_GANANCIA 20u
#define SHUNT_MILLIOHM 50u
#define ADC_BANCOS_COUNT (ADC_BANCO_4 - ADC_BANCO_1 + 1u)


extern ADC_HandleTypeDef hadc1;


volatile uint16_t g_adc_raw[ADC_NODE_COUNT];
static uint8_t g_adc_dma_started = 0u;
static uint8_t g_adc_has_measurements = 0u;
static volatile uint32_t adc_conversion_sequence = 0u;
static volatile uint8_t adc_completed_half = 0u;
static uint32_t adc_processed_sequence = 0u;
static uint32_t adc_processed_ms = 0u;

volatile uint16_t adc_dma_buf[ADC_BUF_LEN];

/********************** internal functions definitions ***********************/
volatile uint8_t adc_half_ready = 0;
volatile uint8_t adc_full_ready = 0;

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc);

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc);

static void adc_process_block(volatile uint16_t *p, uint16_t len);

/********************** internal functions declaration ***********************/

resultado_t adc_init(void)
{
    if (g_adc_dma_started != 0u)
    {
        return RES_OK;
    }

    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
    {
        return Diagnostico_Registrar(ERR_ADC_CALIBRACION, __func__, 0u);
    }

    if (HAL_ADC_Start_DMA(
            &hadc1,
            (uint32_t *)adc_dma_buf,
            ADC_BUF_LEN) == HAL_OK)
    {
        g_adc_dma_started = 1u;
        return RES_OK;
    }
    return Diagnostico_Registrar(ERR_ADC_INICIO, __func__, 0u);
}


void adc_update(void)
{
    /* Tomar solo la mitad completada mas reciente. Evita procesar flags
     * atrasados en orden incorrecto cuando el CLI demora el bucle. */
    uint16_t snapshot[ADC_BUF_LEN / 2];
    uint32_t sequence = adc_conversion_sequence;
    if (sequence == adc_processed_sequence) return;

    uint16_t offset = adc_completed_half ? ADC_BUF_LEN / 2 : 0u;
    for (uint16_t i = 0; i < ADC_BUF_LEN / 2; i++) {
        snapshot[i] = adc_dma_buf[offset + i];
    }
    /* Si DMA cambio de mitad durante la copia, descartar y reintentar. */
    if (sequence != adc_conversion_sequence) return;

    adc_process_block(snapshot, ADC_BUF_LEN / 2);
    adc_processed_sequence = sequence;
    adc_processed_ms = HAL_GetTick();
}

uint32_t adc_get_conversion_sequence(void)
{
    return adc_conversion_sequence;
}

uint32_t adc_get_processed_sequence(void)
{
    return adc_processed_sequence;
}

uint32_t adc_get_processed_ms(void)
{
    return adc_processed_ms;
}


uint16_t adc_get_node_raw(adc_node_t node)
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
        for (uint16_t k = 0; k < ADC_NODE_COUNT; k++)
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
    g_adc_has_measurements = 1u;
}


uint8_t adc_is_dma_started(void)
{
  return g_adc_dma_started;
}

uint8_t adc_has_measurements(void)
{
    /* Indica al menos un bloque procesado, no garantiza frescura. */
    return g_adc_has_measurements;
}

/* Una sola conversion y redondeo final para todas las escalas. */
static uint16_t adc_scaled_value(adc_node_t node, uint32_t num, uint32_t den)
{
    if ((uint32_t)node >= ADC_NODE_COUNT) return 0u;
    const uint64_t divisor = 4095ULL * den;
    return (uint16_t)(((uint64_t)adc_get_node_raw(node) * ADC_VREF_mV * num
                      + divisor / 2u) / divisor);
}

uint16_t adc_get_pin_voltage_mV(adc_node_t node)
{
    return adc_scaled_value(node, 1u, 1u);
}

void adc_get_raw(uint16_t *buffer, uint16_t len)
{
    if (buffer == NULL || len < ADC_NODE_COUNT) {
        Diagnostico_Registrar(buffer == NULL ? ERR_PUNTERO : ERR_BUFFER, __func__, len);
        return;
    }
    for (uint8_t i = 0; i < ADC_NODE_COUNT; i++) {
        buffer[i] = adc_get_node_raw((adc_node_t)i);
    }
}

uint16_t adc_get_node_voltage_mV(adc_node_t node)
{
    if ((uint32_t)node >= ADC_NODE_COUNT) return 0u;
    if (node <= ADC_MUX_BANCO_4) {
        return adc_scaled_value(node, DIV_R1_OHM + DIV_R2_OHM, DIV_R2_OHM);
    }
    if (node <= ADC_BANCO_4) {
        return adc_scaled_value(node, BANCO_R_ENTRADA_OHM,
                                BANCO_R_REALIMENTACION_OHM);
    }
    /* Canales INA: tension de salida, sin divisor. */
    return adc_get_pin_voltage_mV(node);
}

void adc_get_voltages_mV(uint16_t *buffer, uint16_t len)
{
    if (buffer == NULL || len < ADC_NODE_COUNT) {
        Diagnostico_Registrar(buffer == NULL ? ERR_PUNTERO : ERR_BUFFER, __func__, len);
        return;
    }
    for (uint8_t i = 0; i < ADC_NODE_COUNT; i++) {
        buffer[i] = adc_get_node_voltage_mV((adc_node_t)i);
    }
}

uint16_t adc_get_tension_banco_mV(uint8_t banco)
{
    if (banco >= ADC_BANCOS_COUNT) return 0u;
    return adc_get_node_voltage_mV((adc_node_t)(ADC_BANCO_1 + banco));
}

uint16_t adc_get_cell_neg_mV(uint8_t banco)
{
    if (banco >= ADC_BANCOS_COUNT) return 0u;
    return adc_get_node_voltage_mV((adc_node_t)(ADC_MUX_BANCO_1 + banco));
}

uint16_t adc_get_corriente_descarga_mA(void)
{
    return adc_scaled_value(ADC_CORRIENTE_DESCARGA, 1000u,
                            INA_GANANCIA * SHUNT_MILLIOHM);
}

uint16_t adc_get_corriente_carga_mA(void)
{
    return adc_scaled_value(ADC_CORRIENTE_CARGA, 1000u,
                            INA_GANANCIA * SHUNT_MILLIOHM);
}


void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) {
        adc_completed_half = 0u;
        adc_conversion_sequence++;
        adc_half_ready = 1;
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) {
        adc_completed_half = 1u;
        adc_conversion_sequence++;
        adc_full_ready = 1;
    }
}





