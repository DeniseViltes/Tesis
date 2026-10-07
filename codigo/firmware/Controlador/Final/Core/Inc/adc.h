/*
 * adc.h
 *
 *  Created on: 21 jun 2026
 *      Author: ---
 */

#ifndef INC_ADC_H_
#define INC_ADC_H_
#include <stdint.h>



#define ADC_VREF_mV 3310u
#define ADC_SETS   6u
#define ADC_BUF_LEN (ADC_NODE_COUNT * ADC_SETS)


 typedef enum
{
  ADC_MUX_BANCO_1 = 0,
  ADC_MUX_BANCO_2,
  ADC_MUX_BANCO_3,
  ADC_MUX_BANCO_4,
  ADC_BANCO_1,
  ADC_BANCO_2,
  ADC_BANCO_3,
  ADC_BANCO_4,
  ADC_CORRIENTE_DESCARGA,
  ADC_CORRIENTE_CARGA,
  ADC_NODE_COUNT
} adc_node_t;


/*0
12
14
15
1
13
7
5
6
10
 * 
 */


void adc_init(void);
void adc_update(void);
uint8_t adc_has_measurements(void);
uint16_t adc_get_node_raw(adc_node_t node);
void adc_get_raw(uint16_t *buffer, uint16_t len);
#endif /* INC_ADC_H_ */
