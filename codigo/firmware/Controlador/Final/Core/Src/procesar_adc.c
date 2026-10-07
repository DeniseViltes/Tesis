/*
 * procesar_adc.c
 *
 *  Created on: 2 oct 2026
 *      Author: ---
 */

#include "procesar_adc.h"


/*
 * Resistencias de los MUX
 */
#define DIV_R1_OHM 33000u
#define DIV_R2_OHM  8200u

/*
 * Resistencias de los opamps
 */
#define OPAMP_ENTRADA_OHM 39000u
#define OPAMP_REALIMENTACION_OHM 33000u


#define INA_GANANCIA 20u
#define SHUNT 50u  //en mili ohm

#define ADC_CANT_BANCOS (ADC_BANCO_4 - ADC_BANCO_1 + 1u)

static uint16_t scaled_value(adc_node_t node, uint32_t num, uint32_t den)
{
    if ((uint32_t)node >= ADC_NODE_COUNT) return 0u;
    const uint64_t divisor = 4095ULL * den;
    return (uint16_t)(((uint64_t)adc_get_node_raw(node) * ADC_VREF_mV * num
                      + divisor / 2u) / divisor);
}


uint16_t adc_get_pin_voltage_mV(adc_node_t node)
{
    return scaled_value(node, 1u, 1u);
}


uint16_t adc_get_node_voltage_mV(adc_node_t node)
{
    if ((uint32_t)node >= ADC_NODE_COUNT) return 0u;
    if (node <= ADC_MUX_BANCO_4) {
        return scaled_value(node,
                            DIV_R1_OHM + DIV_R2_OHM,
                            DIV_R2_OHM);
    }

    if (node <= ADC_BANCO_4) {
        return scaled_value(node,
                            OPAMP_ENTRADA_OHM,
                            OPAMP_REALIMENTACION_OHM);

    }
    // Para los INA
    return adc_get_pin_voltage_mV(node);
}

int adc_get_voltages_mV(uint16_t *buffer, uint16_t len)
{

    if ((buffer == 0) || (len < ADC_NODE_COUNT))
    {
        return 1;
    }

    for (uint8_t i = 0; i < ADC_NODE_COUNT; i++) {
            buffer[i] = adc_get_node_voltage_mV((adc_node_t)i);
        }
    return 0;
}



uint16_t adc_get_tension_banco_mV(uint8_t banco){
	if (banco >= ADC_CANT_BANCOS) return 0u;
	    return adc_get_node_voltage_mV((adc_node_t)(ADC_BANCO_1 + banco));
}
uint16_t adc_get_cell_neg_mV(uint8_t banco)
{
	if (banco >= ADC_CANT_BANCOS) return 0u;
    return adc_get_node_voltage_mV((adc_node_t)(ADC_MUX_BANCO_1 + banco));
}

uint16_t adc_get_corriente_descarga_mA(void)
{
    return scaled_value(ADC_CORRIENTE_DESCARGA, 1000u,
                            INA_GANANCIA * SHUNT);
}

uint16_t adc_get_corriente_carga_mA(void)
{
    return scaled_value(ADC_CORRIENTE_CARGA, 1000u,
                            INA_GANANCIA * SHUNT);
}


