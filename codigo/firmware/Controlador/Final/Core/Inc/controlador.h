/*
 * controlador.h
 *
 *  Created on: 10 jun 2026
 *      Author: ---
 */

#ifndef INC_CONTROLADOR_H_
#define INC_CONTROLADOR_H_


#include "banco.h"
#include "shift_register.h"
#include "mux.h"


#define CANT_BANCOS 4
//#define CELDAS_POR_BANCO 7
#define PERIODO_DEFAULT 50
#define TURN_ON_DELAY 2

typedef enum {
	REPOSO,
	ACTIVO,
	PRUEBA,
	FALLA
}status_contraldor_t;


typedef enum {
    MODO_SIN_ASIGNAR = 0,
    MODO_CARGA,
    MODO_DESCARGA
} controlador_modo_t;


typedef struct {
	banco_t bancos[CANT_BANCOS];
	uint8_t cant_bancos;
	uint16_t rl;
	uint16_t mediciones[ADC_NODE_COUNT];
	uint16_t mediciones_anteriores[ADC_NODE_COUNT];
	status_contraldor_t estado;
	controlador_modo_t modo;
}controlador_t;

int Controlador_init(uint8_t bancos, uint8_t celdas);
int Controlador_ActivarCircuito(controlador_modo_t modo);
int Controlador_ModoPrueba(void);
void Controlador_Update(void);
void Controlador_AplicarEstados(void);

status_contraldor_t Controlador_GetEstado(void);
uint8_t Controlador_GetCantBancos(void);
uint8_t Controlador_GetCantCeldas(void);
int Controlador_SetResistenciaRL(uint16_t resistencia);
uint16_t Controlador_GetResistenciaRL(void);

/* Comandos: 0 = aceptado, 1 = error; no confirma el estado fisico. */
int Controlador_BypassBanco(uint8_t banco);
int Controlador_ActivarCeldasBanco(uint8_t banco);

void Controlador_Reiniciar(void);
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


int Controlador_DetenerSwitchingBancoBypass(uint8_t banco);
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


uint16_t Controlador_GetTensionSalida(void);
uint16_t Controlador_GetTensionBanco(uint8_t banco);
uint16_t Controlador_GetCellNeg(uint8_t banco);
uint16_t Controlador_GetCorrienteDescarga(void);
uint16_t Controlador_GetCorrienteCarga(void);

#endif /* INC_CONTROLADOR_H_ */
