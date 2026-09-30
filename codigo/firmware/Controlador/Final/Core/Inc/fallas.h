#ifndef INC_FALLAS_H_
#define INC_FALLAS_H_

#include <stdint.h>

/* Umbral nominal; validar tolerancias y tiempo de corte en hardware. */
#define LIMITE_DESCARGA_MA 3000u
#define LIMITE_CARGA_MA 1000u
/* Valores provisionales para ensayos; ajustar con mediciones en placa. */
#define TOLERANCIA_RL_PORCENTAJE 10u
#define TOLERANCIA_RL_MA 50u

/* Limites  LiFePO4; TODO confirmar con la celda.
 * Se considera falla al alcanzar cualquiera de los dos limites. */
#define LIMITE_SUBTENSION_CELDA_MV 2500u
#define LIMITE_SOBRETENSION_CELDA_MV 3600u

typedef enum {
    FALLA_NINGUNA = 0,
    FALLA_SOBRECORRIENTE_DESCARGA,
	FALLA_SOBRECORRIENTE_CARGA,
    FALLA_SUBTENSION_CELDA,
    FALLA_SOBRETENSION_CELDA,
    FALLA_DESCARGA_RL
} falla_t;

/* Requiere mediciones validas y recientes. No acciona la llave ni rearma.
 * Si ambas superan sus limites, devuelve primero la falla de descarga. */
falla_t Fallas_EvaluarCorrientes(uint16_t corrienteDescarga_mA,
                               uint16_t corrienteCarga_mA);

/* Tension entre los terminales de una celda, valida y reciente.
 *
 */
falla_t Fallas_EvaluarTensionCelda(uint16_t tension_mV);
/* I[mA] = V[mV] / RL[ohm]. RL=0: comprobacion no configurada. */
falla_t Fallas_EvaluarDescargaRL(uint16_t tension_mV, uint16_t corriente_mA,
                               uint16_t resistencia_ohm);

#endif /* INC_FALLAS_H_ */
