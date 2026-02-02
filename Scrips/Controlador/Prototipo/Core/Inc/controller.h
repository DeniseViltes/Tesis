#pragma once   //para que solo se incluya una vez el .h

#include "stm32f1xx_hal.h"
#include <stdint.h>

#define NUM_BANKS 3
#define CELLS_PER_BANK 2

typedef enum {
  CELL_OFF = 0,
  CELL_ON  = 1
} cell_state_t;

typedef struct {
  GPIO_TypeDef* port;
  uint16_t pin;
  uint8_t active_high;
} pin_switch_t;


typedef enum {
  CTRL_MODE_MANUAL = 0,
  CTRL_MODE_AUTO   = 1
} ctrl_mode_t;


void Controller_Init(void);
void Controller_SetCell(uint8_t bank, uint8_t cell, cell_state_t st);
cell_state_t Controller_GetCell(uint8_t bank, uint8_t cell);
uint8_t Controller_GetBankSwitch(uint8_t bank);


// Setea tensión objetivo en volt. Rango: 330..990
// Devuelve 1 si OK, 0 si fuera de rango.
uint8_t Controller_SetTargetVoltage_mV(uint16_t v_mV);

// Devuelve 1 si hay tensión configurada, 0 si no.
uint8_t Controller_HasTargetVoltage(void);

// Devuelve la tensión configurada. Si no hay, devuelve 0.
uint16_t Controller_GetTargetVoltage(void);

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




