#pragma once
#include <stdint.h>


typedef enum
{
  ADC_MEAS_CH_0 = 0,
  ADC_MEAS_CH_1,
  ADC_MEAS_CH_4,
  ADC_MEAS_CH_5,
  ADC_MEAS_CH_6,
  ADC_MEAS_CH_7,
  ADC_MEAS_CH_8,
  ADC_MEAS_CH_10,
  ADC_MEAS_CH_11,
  ADC_MEAS_CH_14,
  ADC_MEAS_CH_COUNT
} adc_meas_ch_t;

extern uint16_t g_adc_raw[ADC_MEAS_CH_COUNT];

void adc_init(void);
void adc_update(void);

uint16_t adc_get_raw(adc_meas_ch_t channel);
const uint16_t *adc_get_raw_buffer(void);
uint8_t adc_is_dma_started(void);
