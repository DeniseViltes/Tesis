/*
 * procesar_adc.h
 *
 *  Created on: 4 oct 2026
 *      Author: ---
 */

#ifndef INC_PROCESAR_ADC_H_
#define INC_PROCESAR_ADC_H_

#include "adc.h"


uint16_t adc_get_pin_voltage_mV(adc_node_t node);
uint16_t adc_get_node_voltage_mV(adc_node_t node);
int adc_get_voltages_mV(uint16_t *buffer, uint16_t len);

uint16_t adc_get_tension_banco_mV(uint8_t banco);
uint16_t adc_get_cell_neg_mV(uint8_t banco);
uint16_t adc_get_corriente_descarga_mA(void);
uint16_t adc_get_corriente_carga_mA(void);


#endif /* INC_PROCESAR_ADC_H_ */
