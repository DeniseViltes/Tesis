/*
 * adc.h
 *
 *  Created on: 21 jun 2026
 *      Author: ---
 */

#ifndef INC_ADC_H_
#define INC_ADC_H_
#include <stdint.h>


//acá pongo los nodos a medir
typedef enum
{
  ADC_MUX_BANCO_0 = 0,
  ADC_MUX_BANCO_1,
  ADC_NODE_COUNT
} adc_node_t;





/*
 *typedef enum
{
  ADC_MUX_BANCO_0 = 0,
  ADC_MUX_BANCO_1,
  ADC_MUX_BANCO_2,
  ADC_MUX_BANCO_3,
  ADC_BANCO_0,
  ADC_BANCO_1,
  ADC_BANCO_2,
  ADC_BANCO_3,
  ADC_NODE_COUNT
} adc_node_t;
 */
void adc_init(void);
void adc_update(void);
void adc_get_voltages_mV(uint16_t *buffer, uint16_t len);
uint16_t adc_get_node_voltage_mV(adc_node_t node);
uint16_t adc_get_raw(adc_node_t node);
#endif /* INC_ADC_H_ */
