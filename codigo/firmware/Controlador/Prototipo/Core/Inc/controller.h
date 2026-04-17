#pragma once   //para que solo se incluya una vez el .h

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <adc.h>


#define NUM_BANKS 3
#define CELLS_PER_BANK 2
#define MAX_SQUARE_GROUPS  3


typedef enum {
  SW_OFF = 0,
  SW_ON  = 1
} sw_state_t;




typedef enum {
  CTRL_MODE_MANUAL = 0,
  CTRL_MODE_AUTO   = 1
} ctrl_mode_t;

typedef enum {
  CELL_MODE_STATIC = 0,
  CELL_MODE_SIGNAL
} cell_mode_t;

typedef enum {
	CELL_PHASE_POS = 0,
	CELL_PHASE_NEG
}cell_phase_t;


typedef struct {
	cell_mode_t modo;
	uint8_t id_grupo;
	cell_phase_t fase;
}cell_cfg_t;





void Controller_Init(void);
void Controller_Update(void);
void Controller_SetCell(uint8_t bank, uint8_t cell, sw_state_t st);
sw_state_t Controller_GetCell(uint8_t bank, uint8_t cell);
uint8_t Controller_GetBankSwitch(uint8_t bank);

// Devuelve la tensión configurada. Si no hay, devuelve 0.
uint16_t Controller_GetTargetVoltage(void);

// Setea tensión objetivo en volt. Rango: 330..990
// Devuelve 1 si OK, 0 si fuera de rango.
uint8_t Controller_SetTargetVoltage(float v_mV);




// Setea modo. Para AUTO requiere que haya tensión configurada.
// Devuelve 1 si OK, 0 si no permitido.
uint8_t Controller_SetMode(ctrl_mode_t mode);

ctrl_mode_t Controller_GetMode(void);


/**
 * Carga/actualiza las tensiones de cada banco (en centésimas de volt).
 * Ej: 7.40V -> 7400. Rango típico: 330..990.
 *
 * El controlador se encarga internamente de ordenar y calcular top2.
 */
void Controller_SetBankVoltages_mV(const uint16_t v_bank[NUM_BANKS]);

/*
 * Inicia una señal cuadrada de frecuencia freq en la celda seleccionada
 * la celda comienza apagada
 */
void senial_cuadrada_start(uint8_t bank, uint8_t cell, uint16_t freq);


/*
 * Finaliza una señal cuadrada en la celda seleccionada
 * y apaga la celda
 */
void senial_cuadrada_stop(uint8_t bank, uint8_t cell);

/*
 * Apaga todas las celdas
 */
void apagar_celdas(void);


uint8_t Controller_ConfigSquareGroup(uint8_t id_grupo, uint16_t freq_hz);


/*
 * Prueba de delay (bloqueante)
 */
void Test_ConmutacionConDelay(void);

uint8_t Controller_SetCellSquare(uint8_t bank,
                                 uint8_t cell,
                                 uint8_t id_grupo,
                                 cell_phase_t fase);

/*
 * Para obtener la medición de una celda, (i,j)
 * Para objener la medición de un banco (i,0)
 * con i, j > 0
 */
uint16_t Controller_getMeasurement(uint8_t bank, uint8_t cell);




uint16_t Controller_GetVoltage (void);

uint16_t Tension_medida(void);

