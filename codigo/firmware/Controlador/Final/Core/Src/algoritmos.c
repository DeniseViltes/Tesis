/*
 * algoritmos.c
 *
 *  Created on: 27 oct 2026
 *      Author: ---
 */

#include "algoritmos.h"
#include <stddef.h>


#define VDS 27
#define HISTERESIS 50
/*
 * Mediciones VDS:
 * b1:25mv
 * b2:30mV
 * b3:26mV
 * b4:26mV
 *
 *
 *----> 26,75 promedio
 * Apagagamas:
 * b:valor celda para todos los bancos
 */


uint8_t Algoritmos_OrdenarBancosPorTension(
    const banco_t *bancos,
    uint8_t cantidad_bancos,
    uint8_t *orden)
{
    if (cantidad_bancos == 0u) return 0u;
    if (bancos == NULL || orden == NULL) return 1u;


    uint16_t tensiones_celda[ADC_BANCO_4 - ADC_BANCO_1 + 1u];

    if (cantidad_bancos >
        sizeof(tensiones_celda) / sizeof(tensiones_celda[0]))
        return 1u;

    for (uint8_t b = 0; b < cantidad_bancos; b++) {
        orden[b] = b;

        tensiones_celda[b] = Tension_de_la_celda(
            bancos[b].tension_mV,
            Controlador_GetCellNeg(b));
    }

    // Ordenar los indices de mayor a menor tension de celda.
    for (uint8_t i = 1; i < cantidad_bancos; i++) {
        uint8_t banco = orden[i];
        uint8_t j = i;
        uint16_t tension_celda_A = tensiones_celda[banco];

        while (j > 0u) {
            uint8_t anterior = orden[j - 1u];

            if (tensiones_celda[anterior] >= tension_celda_A)
                break;

            orden[j] = anterior;
            j--;
        }

        orden[j] = banco;
    }

    return 0u;
}


uint8_t Decision_por_umbral(uint32_t tension_actual_mV, uint32_t tension_nueva_mV, uint32_t umbral_mV)
{
    uint32_t diferencia;

    if (tension_nueva_mV >= tension_actual_mV) {
        diferencia = tension_nueva_mV - tension_actual_mV;
    } else {
        diferencia = tension_actual_mV - tension_nueva_mV;
    }

    return diferencia != 0u && diferencia >= umbral_mV;
}




uint16_t Tension_de_la_celda( uint16_t tension_banco, uint16_t cell_neg)
{
    uint32_t caida = (uint32_t)cell_neg + VDS;

    if ((uint32_t)tension_banco <= caida)
        return 0u;

    return (uint16_t)((uint32_t)tension_banco - caida);
}

uint8_t Algoritmos_DebeCambiarBancos(uint32_t actual,uint32_t nueva,
		uint32_t objetivo,uint32_t error_entrada,uint32_t error_salida,
		uint32_t umbral_cambio,histeresis_t *estado){

}

static uint32_t diferencia_absoluta(uint32_t a, uint32_t b)
{
    return a >= b ? a - b : b - a;
}




