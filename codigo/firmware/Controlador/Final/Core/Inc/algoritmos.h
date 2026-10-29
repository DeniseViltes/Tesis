#ifndef INC_ALGORITMOS_H_
#define INC_ALGORITMOS_H_

#include "controlador.h"


typedef struct {
    uint8_t corrigiendo;
} histeresis_t;

uint8_t Algoritmos_DebeCambiarBancos(uint32_t actual,uint32_t nueva,
		uint32_t objetivo,uint32_t error_entrada,uint32_t error_salida,
		uint32_t umbral_cambio,histeresis_t *estado);




/* Guarda indices de bancos de mayor a menor tension, sin mover bancos.
 * En empates conserva el orden original. orden debe tener espacio para
 * cantidad_bancos elementos y no solaparse con bancos.
 * Retorna 0 en exito, 1 ante punteros nulos con cantidad distinta de cero. */
uint8_t Algoritmos_OrdenarBancosPorTension(const banco_t *bancos,
                                         uint8_t cantidad_bancos,
                                         uint8_t *orden);


uint16_t Tension_de_la_celda(
    uint16_t tension_banco,
    uint16_t cell_neg);


uint8_t Decision_por_umbral(uint32_t tension_actual_mV, uint32_t tension_nueva_mV, uint32_t umbral_mV);

uint8_t Algoritmos_DebeCambiarBancos(uint32_t actual,uint32_t nueva,
		uint32_t objetivo,uint32_t error_entrada,uint32_t error_salida,
		uint32_t umbral_cambio,histeresis_t *estado){

}
