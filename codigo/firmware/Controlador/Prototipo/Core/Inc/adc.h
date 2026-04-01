#pragma once
#include <stdint.h>


/*typedef enum
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
} adc_meas_ch_t;*/


typedef enum
{
  ADC_NODE_CELL_11 = 0,
  ADC_NODE_CELL_12,
  ADC_NODE_CELL_21,
  ADC_NODE_CELL_22,
  ADC_NODE_CELL_31,
  ADC_NODE_CELL_32,
  ADC_NODE_OUT_POS,
  ADC_NODE_OUT_NEG,
  ADC_NODE_BUS_1,
  ADC_NODE_BUS_2,
  ADC_NODE_COUNT
} adc_node_t;




void adc_init(void);
void adc_update(void);

uint16_t adc_get_raw(adc_node_t node);
uint8_t adc_is_dma_started(void);
uint16_t adc_get_pin_voltage(adc_node_t node);
uint16_t adc_get_meas_voltage(adc_node_t node);

