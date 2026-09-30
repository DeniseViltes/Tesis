/*
 * adc.h
 *
 *  Created on: 21 jun 2026
 *      Author: ---
 */

#ifndef INC_ADC_H_
#define INC_ADC_H_
#include <stdint.h>
#include "diagnostico.h"


//acá pongo los nodos a medir
/*typedef enum
{
  ADC_MUX_BANCO_1 = 0,
  ADC_MUX_BANCO_1,
  ADC_NODE_COUNT
} adc_node_t;

*/


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


/* El enum conserva el orden de ranks DMA (indice = rank - 1). */
resultado_t adc_init(void);
void adc_update(void);
uint8_t adc_is_dma_started(void);
/* Hay un bloque procesado; no implica alimentacion ni frescura verificadas. */
uint8_t adc_has_measurements(void);
/* Secuencia de mitades DMA completadas y del ultimo bloque procesado.
 * Una diferencia >= 2 desde una marca garantiza un bloque entero posterior
 * a esa marca. Consultar desde el bucle principal. */
uint32_t adc_get_conversion_sequence(void);
uint32_t adc_get_processed_sequence(void);
uint32_t adc_get_processed_ms(void);

/* Cuentas ADC sin conversion: 0..4095. */
uint16_t adc_get_node_raw(adc_node_t node);
void adc_get_raw(uint16_t *buffer, uint16_t len);
/* Tension en el pin del micro, sin corregir el circuito externo. */
uint16_t adc_get_pin_voltage_mV(adc_node_t node);
/* MUX: divisor; banco: diferencial; INA: tension de salida en mV. */
uint16_t adc_get_node_voltage_mV(adc_node_t node);
void adc_get_voltages_mV(uint16_t *buffer, uint16_t len);

/* Banco: indice 0..3. Lecturas del ultimo bloque DMA procesado.
 * CellNeg es el terminal negativo seleccionado, no la tension de la celda.
 * Tras cambiar MUX se requiere estabilizacion y un bloque nuevo. */
uint16_t adc_get_tension_banco_mV(uint8_t banco);
uint16_t adc_get_cell_neg_mV(uint8_t banco);
uint16_t adc_get_corriente_descarga_mA(void);
uint16_t adc_get_corriente_carga_mA(void);
#endif /* INC_ADC_H_ */
