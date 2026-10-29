/*
 * controlador.h
 *
 *  Created on: 10 jun 2026
 *      Author: ---
 */

#ifndef INC_CONTROLADOR_H_
#define INC_CONTROLADOR_H_


#include "banco.h"
#include "fallas.h"
#include "shift_register.h"
#include "mux.h"


#define CANT_BANCOS 4
//#define CELDAS_POR_BANCO 7
#define PERIODO_DEFAULT 50
#define TURN_ON_DELAY 2
/* Valor inicial: ajustar al asentamiento medido del circuito analogico. */
#define TENSION_ASENTAMIENTO_MS 2u
#define UMBRAL_CAMBIO 20


typedef enum {
	REPOSO,
	ACTIVO,
	PRUEBA,
	FALLA
}controlador_status_t;


typedef enum {
    MODO_SIN_ASIGNAR = 0,
    MODO_CARGA,
    MODO_DESCARGA
} controlador_modo_t;


typedef enum {
    CONTROL_MANUAL = 0,
    CONTROL_AUTOMATICO
} controlador_control_t;

typedef struct {
	banco_t bancos[CANT_BANCOS];
	uint8_t cant_bancos;
	uint16_t rl;
	uint16_t mediciones[ADC_NODE_COUNT];
	uint16_t mediciones_anteriores[ADC_NODE_COUNT];
	controlador_status_t estado;
	controlador_modo_t modo;
	controlador_control_t control;
	uint16_t tension_objetivo;
	falla_t falla;
}controlador_t;

int Controlador_init(uint8_t bancos, uint8_t celdas);
int Controlador_ActivarCircuito(controlador_modo_t modo);
int Controlador_ModoPrueba(void);
int Controlador_Update(void);
falla_t Controlador_GetFalla(void);

void Controlador_AplicarEstados(void);

controlador_status_t Controlador_GetEstado(void);
uint8_t Controlador_GetCantBancos(void);
uint8_t Controlador_GetCantCeldas(void);
int Controlador_SetResistenciaRL(uint16_t resistencia);
uint16_t Controlador_GetResistenciaRL(void);

/* Comandos: 0 = aceptado, 1 = error; no confirma el estado fisico. */
int Controlador_BypassBanco(uint8_t banco);
int Controlador_ActivarCeldasBanco(uint8_t banco);

/* 0 = aceptado, 1 = error. Reiniciar requiere init; apagar, ACTIVO o PRUEBA. */
int Controlador_Reiniciar(void);
int Controlador_Apagar(void);
uint8_t Controlador_GetEstadoCelda(uint8_t banco, uint8_t celda);
uint8_t Controlador_GetEstadoBanco(uint8_t banco);


int Controlador_EncenderCelda(uint8_t banco, uint8_t celda);
int Controlador_ApagarCelda(uint8_t banco, uint8_t celda);
void Controlador_ActualizarSwitching(void);

int Controlador_HayCambios(void);

void Controlador_Tick1ms(void);

int Controlador_IniciarSwitchingCelda(uint8_t banco, uint8_t celda);

int Controlador_ModificarModoCelda(uint8_t banco, uint8_t celda, char modo);
int Controlador_ModificarModo(uint8_t banco, char modo);

int Controlador_IniciarSwitchingBanco(uint8_t banco);


void Controlador_ActualizarEstados(void);


controlador_control_t Controlador_GetControl(void);
int Controlador_PararSwitchingCelda(uint8_t banco, uint8_t celda);


int Controlador_ModificarPeriodo(uint16_t periodo);
int Controlador_SeleccionarMux(uint8_t celda);

/*
 * Mide el canal de mux previamente seleccionado
 * Primero es necesario seleccionar un canal de mux (una celda)
 */
uint16_t Controlador_MedirCellNeg(uint8_t banco);

uint16_t Controlador_GetMedicion(adc_node_t nodo);
void Controlador_CargarMediciones(void);

/*
 * Salida calculada a partir de la suma de los bancos activos;
 */
uint16_t Controlador_GetTensionSalidaOpamp(void);

/*
 * Salida calculada mendiante la ley de ohm
 * Apartir del input de resistencia y la corriente medida
 */
uint16_t Controlador_GetTensionSalida(void);
uint16_t Controlador_GetTensionBanco(uint8_t banco);
uint16_t Controlador_GetCellNeg(uint8_t banco);
/* Ultimo barrido completo. Indices desde cero; 1 = dato disponible.
 * Si devuelve 0, no modifica *valor_mV. */
int Controlador_GetCellNegBarrido(uint8_t banco, uint8_t celda, uint16_t *valor_mV);
/* Consultar primero la disponibilidad del barrido. */
uint32_t Controlador_GetAntiguedadBarridoMs(void);
uint8_t Controlador_HayMediciones(void);
uint16_t Controlador_GetCorrienteDescarga(void);
uint16_t Controlador_GetCorrienteCarga(void);



int Controlador_IniciarModoAutomatico(void);
int Controlador_ApagarModoAutomatico(void);
int Controlador_CalcularCantidadEncendidos(uint16_t tension_objetivo, uint8_t *orden, uint16_t *tension_lograda);

int Controlador_EncenderBancosAutomatico(uint16_t tension_objetivo);
int Controlador_SetTensionObjetivo(uint16_t tension);

#endif /* INC_CONTROLADOR_H_ */
