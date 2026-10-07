/*
 * fallas.c
 *
 *  Created on: 7 oct 2026
 *      Author: ---
 */

#include "fallas.h"
/*
falla_t Fallas_EvaluarDescargaRL(uint16_t tension_mV, uint16_t corriente_mA,
                               uint16_t resistencia_ohm)
{
    if (resistencia_ohm == 0u) return FALLA_NINGUNA;
    uint32_t esperada_mA = (uint32_t)tension_mV / resistencia_ohm;
    uint32_t diferencia = corriente_mA > esperada_mA
        ? corriente_mA - esperada_mA : esperada_mA - corriente_mA;
    uint32_t tolerancia = TOLERANCIA_RL_MA +
        esperada_mA * TOLERANCIA_RL_PORCENTAJE / 100u;
    return diferencia > tolerancia ? FALLA_DESCARGA_RL : FALLA_NINGUNA;
}*/

falla_t Fallas_EvaluarTensionCelda(uint16_t tension_mV)
{
    if (tension_mV <= LIMITE_SUBTENSION_CELDA_MV) {
        return FALLA_SUBTENSION_CELDA;
    }
    if (tension_mV >= LIMITE_SOBRETENSION_CELDA_MV) {
        return FALLA_SOBRETENSION_CELDA;
    }
    return FALLA_NINGUNA;
}

falla_t Fallas_EvaluarCorrientes(uint16_t corrienteDescarga_mA,
                               uint16_t corrienteCarga_mA)
{
    if (corrienteDescarga_mA >= LIMITE_DESCARGA_MA) {
        return FALLA_SOBRECORRIENTE_DESCARGA;
    }
    if (corrienteCarga_mA >= LIMITE_CARGA_MA) {
        return FALLA_SOBRECORRIENTE_CARGA;
    }
    return FALLA_NINGUNA;
}

