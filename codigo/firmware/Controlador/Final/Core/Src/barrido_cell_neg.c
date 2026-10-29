/*
 * barrido_cell_neg.c
 *
 *  Created on: 26 oct 2026
 *      Author: ---
 */

#include "barrido_cell_neg.h"
#include "procesar_adc.h"
#include "main.h"
#include <string.h>

#define BARRIDO_MAX_BANCOS (ADC_MUX_BANCO_4 - ADC_MUX_BANCO_1 + 1u)

typedef enum { ESPERAR_INTERVALO, ESPERAR_ASENTAMIENTO, ESPERAR_ADC } estado_t;

typedef struct {
    banco_t *bancos;
    uint8_t cantidad_bancos;
    uint8_t cantidad_celdas;
    uint8_t celda_actual;
    uint8_t hay_barrido;
    estado_t estado;
    uint32_t inicio_ms, seleccion_ms, adc_base, actualizacion_ms;
    uint16_t pendientes[BARRIDO_MAX_BANCOS][CELDAS_POR_BANCO];
    uint16_t mediciones[BARRIDO_MAX_BANCOS][CELDAS_POR_BANCO];
} barrido_cell_neg_t;

static barrido_cell_neg_t barrido;

void BarridoCellNeg_Init(banco_t *configurados, uint8_t cantidad)
{
    barrido.bancos = configurados;
    barrido.cantidad_bancos = (configurados != NULL && cantidad <= BARRIDO_MAX_BANCOS)
        ? cantidad : 0u;
    barrido.cantidad_celdas = 0u;
    for (uint8_t b = 0; b < barrido.cantidad_bancos; b++) {
        if (barrido.bancos[b].cant_celdas > CELDAS_POR_BANCO) {
            barrido.cantidad_bancos = 0u;
            break;
        }
        if (barrido.bancos[b].cant_celdas > barrido.cantidad_celdas)
            barrido.cantidad_celdas = barrido.bancos[b].cant_celdas;
    }
    barrido.estado = ESPERAR_INTERVALO;
    barrido.inicio_ms = HAL_GetTick();
    barrido.actualizacion_ms = 0u;
    barrido.celda_actual = barrido.hay_barrido = 0u;
    memset(barrido.pendientes, 0, sizeof(barrido.pendientes));
    memset(barrido.mediciones, 0, sizeof(barrido.mediciones));
}

static void seleccionar_celda(void)
{
    for (uint8_t b = 0; b < barrido.cantidad_bancos; b++) {
        if (barrido.celda_actual < barrido.bancos[b].cant_celdas) {
            Banco_SeleccionarCeldaMux(&barrido.bancos[b], barrido.celda_actual);
            break;
        }
    }
    barrido.seleccion_ms = HAL_GetTick();
    barrido.estado = ESPERAR_ASENTAMIENTO;
}

void BarridoCellNeg_Update(void)
{
    if (barrido.cantidad_bancos == 0u || barrido.cantidad_celdas == 0u) return;
    uint32_t ahora = HAL_GetTick();

    if (barrido.estado != ESPERAR_INTERVALO &&
        (uint32_t)(ahora - barrido.inicio_ms) >= INTERVALO_MEDICION) {
        barrido.estado = ESPERAR_INTERVALO;
    }

    switch (barrido.estado) {
    case ESPERAR_INTERVALO:
        if ((uint32_t)(ahora - barrido.inicio_ms) < INTERVALO_MEDICION)
            return;
        if (!adc_is_dma_started() || !adc_has_measurements()) return;

        barrido.inicio_ms += ((uint32_t)(ahora - barrido.inicio_ms) /
            INTERVALO_MEDICION) * INTERVALO_MEDICION;
        barrido.celda_actual = 0u;
        seleccionar_celda();
        break;
    case ESPERAR_ASENTAMIENTO:
        if ((uint32_t)(ahora - barrido.seleccion_ms) <
            BARRIDO_CELL_NEG_ASENTAMIENTO_MS) return;
        barrido.adc_base = adc_get_completed_sequence();
        barrido.estado = ESPERAR_ADC;
        break;
    case ESPERAR_ADC: {
        /* El primer bloque puede haber empezado antes del asentamiento. */
        uint32_t avance = adc_get_processed_sequence() - barrido.adc_base;
        if (avance < 2u || avance >= 0x80000000u) return;
        for (uint8_t b = 0; b < barrido.cantidad_bancos; b++) {
            if (barrido.celda_actual < barrido.bancos[b].cant_celdas)
                barrido.pendientes[b][barrido.celda_actual] = adc_get_cell_neg_mV(b);
        }
        barrido.celda_actual++;
        if (barrido.celda_actual < barrido.cantidad_celdas) {
            seleccionar_celda();
        } else {
            memcpy(barrido.mediciones, barrido.pendientes, sizeof(barrido.mediciones));
            barrido.actualizacion_ms = ahora;
            barrido.hay_barrido = 1u;
            barrido.estado = ESPERAR_INTERVALO;
        }
        break;
    }
    }
}

uint8_t BarridoCellNeg_Ocupado(void)
{
    return barrido.estado != ESPERAR_INTERVALO;
}

int BarridoCellNeg_GetMedicion(uint8_t banco, uint8_t celda, uint16_t *valor)
{
    if (!barrido.hay_barrido || valor == NULL || banco >= barrido.cantidad_bancos ||
        celda >= barrido.bancos[banco].cant_celdas) return 0;
    *valor = barrido.mediciones[banco][celda];
    return 1;
}

uint32_t BarridoCellNeg_GetActualizacionMs(void)
{
    return barrido.actualizacion_ms;
}


