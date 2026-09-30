/*
 * controlador.h
 *
 *  Created on: 10 jun 2026
 *      Author: ---
 */

#ifndef INC_CONTROLADOR_H_
#define INC_CONTROLADOR_H_


#include "banco.h"
#include "diagnostico.h"
#include "shift_register.h"
#include "mux.h"


#define CANT_BANCOS 4
//#define CELDAS_POR_BANCO 7
#define PERIODO_DEFAULT 50

typedef enum {
	INIT = 0,
	DESCARGA,
	CARGA,
	REPOSO,
	ABIERTO,
	FALLA,
	PRUEBA
} controlador_estado_t;

typedef struct {
	controlador_estado_t estado;
	banco_t bancos[CANT_BANCOS];
	uint8_t cant_bancos;
	uint16_t mediciones[ADC_NODE_COUNT];
	uint16_t mediciones_anteriores[ADC_NODE_COUNT];
	uint16_t resistencia_carga; /* RL en ohm; 0 = no configurada. */
}controlador_t;




/* Las operaciones retornan RES_OK o un error registrado en diagnostico. */
/* Celdas por banco; rangos: 1..CANT_BANCOS y 1..CELDAS_POR_BANCO. */
resultado_t Controlador_init(uint8_t bancos, uint8_t celdas);
resultado_t Controlador_SetResistenciaCarga(uint16_t resistencia_ohm);
uint16_t Controlador_GetResistenciaCarga(void);
uint8_t Controlador_GetCantBancos(void);
uint8_t Controlador_GetCantCeldas(void);
void Controlador_Update(void);
resultado_t Controlador_EntrarReposo(void);
resultado_t Controlador_EntrarPrueba(void);
controlador_estado_t Controlador_GetEstado(void);
typedef enum {
    BARRIDO_SIN_DATOS = 0,
    BARRIDO_EN_CURSO,
    BARRIDO_COMPLETO,
    BARRIDO_CANCELADO,
    BARRIDO_TIMEOUT
} barrido_estado_t;

typedef struct {
    uint16_t cell_neg_mV;       /* Respecto de GND; NO tension de la celda. */
    uint16_t banco_mV;         /* Diferencial del mismo bloque ADC. */
    uint32_t instante_ms;      /* Instante de procesamiento. */
    uint8_t valida;            /* Adquisicion completa, no diagnostico electrico. */
    uint8_t limite_adc;        /* CellNeg en 0 o 4095: fuera de rango posible. */
    uint8_t celda_on;
    uint8_t bypass_on;
} medicion_celda_t;

/* Iniciar solo con matriz habilitada y ADC arrancado. No acciona switches.
 * El MUX queda en la ultima celda recorrida. API desde el bucle principal. */
uint8_t Controlador_IniciarBarrido(void);
void Controlador_ActualizarBarrido(void);
void Controlador_CancelarBarrido(void);
barrido_estado_t Controlador_GetEstadoBarrido(void);
uint8_t Controlador_GetMedicionCelda(uint8_t banco, uint8_t celda,
                                    medicion_celda_t *medicion);


/* Evaluacion y transiciones de la maquina de estados. */
int Controlador_EvaluarFallas(void);
void Controlador_EvaluarEstado(void);
resultado_t Controlador_CambiarEstado(controlador_estado_t nuevo_estado);

void Controlador_AplicarEstados(void);


resultado_t Controlador_BypassBanco(uint8_t banco);
resultado_t Controlador_ActivarCeldasBanco(uint8_t banco);

resultado_t Controlador_EstadoAbierto(void);
uint8_t Controlador_GetEstadoCelda(uint8_t banco, uint8_t celda);
uint8_t Controlador_GetEstadoBanco(uint8_t banco);


resultado_t Controlador_EncenderCelda(uint8_t banco, uint8_t celda);
resultado_t Controlador_ApagarCelda(uint8_t banco, uint8_t celda);
void Controlador_ActualizarSwitching(void);

int Controlador_HayCambios(void);

void Controlador_Tick1ms(void);

resultado_t Controlador_IniciarSwitchingCelda(uint8_t banco, uint8_t celda);

resultado_t Controlador_ModificarModoCelda(uint8_t banco, uint8_t celda, char modo);
resultado_t Controlador_ModificarModo(uint8_t banco, char modo);

resultado_t Controlador_IniciarSwitchingBanco(uint8_t banco);


void Controlador_ActualizarEstados(void);


resultado_t Controlador_DetenerSwitchingBancoBypass(uint8_t banco);
resultado_t Controlador_PararSwitchingCelda(uint8_t banco, uint8_t celda);


resultado_t Controlador_ModificarPeriodo(uint16_t periodo);
resultado_t Controlador_SeleccionarMux(uint8_t celda);

/*
 * Mide el canal de mux previamente seleccionado
 * Primero es necesario seleccionar un canal de mux (una celda)
 */
uint16_t Controlador_MedirCellNeg(uint8_t banco);

/* Consultas del ultimo bloque ADC procesado; bancos configurados, base cero. */
uint16_t Controlador_GetTensionSalida_mV(void);
uint16_t Controlador_GetTensionBanco_mV(uint8_t banco);
uint16_t Controlador_GetCellNeg_mV(uint8_t banco);
uint16_t Controlador_GetCorrienteDescarga_mA(void);
uint16_t Controlador_GetCorrienteCarga_mA(void);

/* Snapshot cargado por Controlador_CargarMediciones(), siempre en mV. */
uint16_t Controlador_GetMedicion(adc_node_t nodo);
void Controlador_CargarMediciones(void);

#endif /* INC_CONTROLADOR_H_ */
