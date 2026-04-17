
 /*
#include "controller.h"
#include "main.h"
#include <stdint.h>
#include <adc.h>




// ======================= ESTADO CELDAS =======================



static sw_state_t cell_state[NUM_BANKS][CELLS_PER_BANK];

static cell_cfg_t cell_cfg[NUM_BANKS][CELLS_PER_BANK];


typedef enum
{
  CELL_11 = 0,
  CELL_12,
  CELL_21,
  CELL_22,
  CELL_31,
  CELL_32
}celdas_id_t;

typedef enum
{
  BANK_1,
  BANK_2,
  BANK_3
} bancos_id_t;


typedef struct {
  uint8_t  enabled;
  uint16_t ticks;   // en ticks de 1 ms
  uint16_t contador;
  uint8_t  fase;               // si está en on u off
} config_cuadrada_t;

static config_cuadrada_t square_groups[MAX_SQUARE_GROUPS];

typedef struct {
  GPIO_TypeDef* port;
  uint16_t pin;
  uint8_t active_high;
} pin_switch_t;



typedef struct {
    //const char *nombre;
    uint8_t id_banco;
    uint8_t id_celda;
    pin_switch_t sw;
    sw_state_t state;

    uint16_t tension;
    uint16_t vbus_pos;
    uint16_t vbus_neg;
} cell_t;



typedef struct {
    //const char *name;
    uint8_t id_banco;
    pin_switch_t sw;
    sw_state_t state;

    uint16_t tension;
    uint16_t vbus_pos;
    uint16_t vbus_neg;
} bank_t;


typedef struct {
	const char *name;
    bank_t bancos[NUM_BANKS];
    cell_t celdas[NUM_BANKS][CELLS_PER_BANK];
    ctrl_mode_t mode;
    uint16_t target_mV;
} controller_t;


static controller_t controlador;


static bank_v_t bank_rank[NUM_BANKS];

#define UMBRAL_mV 10  // 10mV

*/

//-----------------FUNCIONES-------------------
/*
void Controller_SetMode(ctrl_mode_t mode);
ctrl_mode_t Controller_GetMode(void);


void Controller_SetBank(uint8_t bank, uint8_t enable);
uint8_t Controller_GetBank(uint8_t bank);

void Controller_SetCell(uint8_t bank, uint8_t cell, sw_state_t state);
sw_state_t Controller_GetCell(uint8_t bank, uint8_t cell);

void Controller_ApplyOutputs(void);

void Controller_UpdateMeasurements(const uint16_t *adc_mV);

static void Controller_RunAutoLogic(void);
*/
// ======================= GPIO WRITE ======================= *

/*
static void gpio_write(const pin_switch_t* g, uint8_t on)
{
  GPIO_PinState ps;

  if (g->active_high)
    ps = on ? GPIO_PIN_SET : GPIO_PIN_RESET;
  else
    ps = on ? GPIO_PIN_RESET : GPIO_PIN_SET;

  HAL_GPIO_WritePin(g->port, g->pin, ps);
}


void Controller_Init(void)
{

	controlador.mode = CTRL_MODE_MANUAL;


    controlador.banks[0] = (bank_t){
        .name = "B1",
        .bank_id = 0,
        .sw = {Vc_1_GPIO_Port, Vc_1_Pin, 1},
        .seleccionado = 0,
        .sw_state = SW_OFF
    };

    controlador.banks[1] = (bank_t){
        .name = "B2",
        .bank_id = 1,
        .sw = {Vc_2_GPIO_Port, Vc_2_Pin, 1},
        .seleccionado = 0,
        .sw_state = SW_OFF
    };

    controlador.banks[2] = (bank_t){
        .name = "B3",
        .bank_id = 2,
        .sw = {Vc_3_GPIO_Port, Vc_3_Pin, 1},
        .seleccionado = 0,
        .sw_state = SW_OFF
    };

    // ===== CELDAS =====

    // Banco 1
    controlador.cells[0][0] = (cell_t){
        .name = "C11",
        .bank_id = 0,
        .cell_id = 0,
        .sw = {Vc_11_GPIO_Port, Vc_11_Pin, 1},
        .cmd_state = SW_OFF,
        .state = SW_OFF
    };

    controlador.cells[0][1] = (cell_t){
        .name = "C12",
        .bank_id = 0,
        .cell_id = 1,
        .sw = {Vc_12_GPIO_Port, Vc_12_Pin, 1},
        .cmd_state = SW_OFF,
        .state = SW_OFF
    };

    // Banco 2
    controlador.cells[1][0] = (cell_t){
        .name = "C21",
        .bank_id = 1,
        .cell_id = 0,
        .sw = {Vc_21_GPIO_Port, Vc_21_Pin, 1},
        .cmd_state = SW_OFF,
        .state = SW_OFF
    };

    controlador.cells[1][1] = (cell_t){
        .name = "C22",
        .bank_id = 1,
        .cell_id = 1,
        .sw = {Vc_22_GPIO_Port, Vc_22_Pin, 1},
        .cmd_state = SW_OFF,
        .state = SW_OFF
    };

    // Banco 3
    controlador.cells[2][0] = (cell_t){
        .name = "C31",
        .bank_id = 2,
        .cell_id = 0,
        .sw = {Vc_31_GPIO_Port, Vc_31_Pin, 1},
        .cmd_state = SW_OFF,
        .state = SW_OFF
    };

    controlador.cells[2][1] = (cell_t){
        .name = "C32",
        .bank_id = 2,
        .cell_id = 1,
        .sw = {Vc_32_GPIO_Port, Vc_32_Pin, 1},
        .cmd_state = SW_OFF,
        .state = SW_OFF
    };
}
*/


//-----------------BASICAS-------------------

/*
static void Controller_UpdateBank(uint8_t bank)
{
  uint8_t both_off =
    (cell_state[bank][0] == SW_OFF) &&
    (cell_state[bank][1] == SW_OFF);

  gpio_write(&bank_sw[bank], both_off);
}

static void _controller_SetCell_ON(uint8_t bank, uint8_t cell)
{
  cell_state[bank][cell] = SW_ON;
  Controller_UpdateBank(bank);

  gpio_write(&cell_sw[bank][cell], 1);

}

static void _controller_SetCell_OFF(uint8_t bank, uint8_t cell)
{
  cell_state[bank][cell] = SW_OFF;
  gpio_write(&cell_sw[bank][cell], 0);

  Controller_UpdateBank(bank);
}



void Controller_SetBank(uint8_t bank, uint8_t enable){
		controlador.bancos[bank].seleccionado = enable ? 1 : 0;
}



uint8_t Controller_GetBank(uint8_t bank){
	return controlador.bancos[bank].seleccionado;
}

void Controller_ApplyOutputs (void){
	for (uint8_t b = 0; b < BANKS; b++){
		if (controlador.bancos[b].seleccionado){
			for (uint8_t c = 0; c < CELLS_PER_BANK; c++){
				controlador.celdas[b][c].cmd_state = SW_ON;
			}
		} else {
		        // banco inactivo → apagar celdas
		        for (uint8_t c = 0; c < CELLS_PER_BANK; c++) {
		        	controlador.celdas[b][c].cmd_state = SW_OFF;
		        }
		}
	}

}

*/


