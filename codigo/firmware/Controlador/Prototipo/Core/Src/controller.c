#include "controller.h"
#include "main.h"
#include <stdint.h>
#include <adc.h>




/* ======================= ESTADO CELDAS ======================= */



static sw_state_t cell_state[NUM_BANKS][CELLS_PER_BANK];

static cell_cfg_t cell_cfg[NUM_BANKS][CELLS_PER_BANK];


//asi o separadas las celdas de los bancos?


typedef enum
{
  CELL_11 = 0,
  CELL_12,
  CELL_21,
  CELL_22,
  CELL_31,
  CELL_32,
  BANK_1,
  BANK_2,
  BANK_3
} module_t;


typedef struct {
  uint8_t  enabled;
  uint16_t ticks;   // en ticks de 1 ms
  uint16_t counter;
  uint8_t  phase;               // si está en on u off
} square_group_t;

static square_group_t square_groups[MAX_SQUARE_GROUPS];

typedef struct {
  GPIO_TypeDef* port;
  uint16_t pin;
  uint8_t active_high;
  module_t id;
} pin_switch_t;


/* ======================= MAPEOS SWITCHES ======================= */

// Celdas
static pin_switch_t cell_sw[NUM_BANKS][CELLS_PER_BANK] = {
  // Banco 1
  {
    { Vc_11_GPIO_Port, Vc_11_Pin, 1, CELL_11},
    { Vc_12_GPIO_Port, Vc_12_Pin, 1, CELL_12 }
  },
  // Banco 2
  {
    { Vc_21_GPIO_Port, Vc_21_Pin, 1, CELL_21},
    { Vc_22_GPIO_Port, Vc_22_Pin, 1, CELL_22}
  },
  // Banco 3
  {
    { Vc_31_GPIO_Port, Vc_31_Pin, 1, CELL_31},
    { Vc_32_GPIO_Port, Vc_32_Pin, 1 , CELL_32}
  }
};

// Bancos
static pin_switch_t bank_sw[NUM_BANKS] = {
  { Vc_1_GPIO_Port, Vc_1_Pin, 1, BANK_1 },
  { Vc_2_GPIO_Port, Vc_2_Pin, 1, BANK_2 },
  { Vc_3_GPIO_Port, Vc_3_Pin, 1, BANK_3 }
};


/* ======================= NODOS DE MEDICION ======================= */


typedef struct {
  adc_node_t nodo;
  uint16_t v_mV;   // miliVolt
  module_t cell;
} cell_v_t;

typedef struct {
  adc_node_t bus_pos;
  adc_node_t bus_neg;
  uint16_t v_mV;   // miliVolt
  module_t bank;
} bank_v_t;


static bank_v_t bank_nodes[NUM_BANKS] = {
    {ADC_NODE_BUS_1 , ADC_NODE_OUT_NEG, 3300, BANK_1 },
    {ADC_NODE_BUS_2 , ADC_NODE_BUS_1, 3300, BANK_2 },
    {ADC_NODE_OUT_POS , ADC_NODE_BUS_2, 3300, BANK_3 }
};

#define TOTAL_CELLS (CELLS_PER_BANK*NUM_BANKS)

static cell_v_t cell_nodes[TOTAL_CELLS] = {
    {ADC_NODE_CELL_11,  3300, CELL_11 },
    {ADC_NODE_CELL_12,  3300, CELL_12 },
    {ADC_NODE_CELL_21,  3300, CELL_21 },
    {ADC_NODE_CELL_22,  3300, CELL_22 },
    {ADC_NODE_CELL_31,  3300, CELL_31 },
	{ADC_NODE_CELL_32,  3300, CELL_32 }
};

/*typedef struct {
  uint8_t  bank;   // 0..NUM_BANKS-1
  uint16_t v_mV;   // miliVolt
} bank_v_t;

static uint16_t bank_voltage[NUM_BANKS] = {0};*/


static bank_v_t bank_rank[NUM_BANKS];


/* ======================= VARIABLES DE TENSION ======================= */

static ctrl_mode_t g_ctrl_mode = CTRL_MODE_MANUAL;
static uint16_t g_vtarget_mV = 3300;     // mV tension elegida
static uint16_t tension_controlada = 0;
static uint16_t tension_medida = 0;


#define UMBRAL_mV 10  // 10mV

/* ======================= GPIO WRITE ======================= */

static void gpio_write(const pin_switch_t* g, uint8_t on)
{
  GPIO_PinState ps;

  if (g->active_high)
    ps = on ? GPIO_PIN_SET : GPIO_PIN_RESET;
  else
    ps = on ? GPIO_PIN_RESET : GPIO_PIN_SET;

  HAL_GPIO_WritePin(g->port, g->pin, ps);
}

/* ======================= LOGICA BANCO ======================= */

static void Controller_UpdateBank(uint8_t bank)
{
  // Switch del banco ON solo si AMBAS celdas están OFF
  uint8_t both_off =
    (cell_state[bank][0] == SW_OFF) &&
    (cell_state[bank][1] == SW_OFF);

  gpio_write(&bank_sw[bank], both_off);
}

/* ======================= INIT ======================= */

void Controller_Init(void)
{
  for (uint8_t b = 0; b < NUM_BANKS; b++) {
    for (uint8_t c = 0; c < CELLS_PER_BANK; c++) {
      cell_state[b][c] = SW_OFF;

      cell_cfg[b][c].modo = CELL_MODE_STATIC;
      cell_cfg[b][c].id_grupo = 0;
      cell_cfg[b][c].fase = CELL_PHASE_POS;

      gpio_write(&cell_sw[b][c], 0);
    }

    Controller_UpdateBank(b);
  }

  for (uint8_t g = 0; g < MAX_SQUARE_GROUPS; g++) {
    square_groups[g].enabled = 0;
    square_groups[g].ticks = 0;
    square_groups[g].counter = 0;
    square_groups[g].phase = 0;
  }

  for (uint8_t i = 0; i < NUM_BANKS; i++) {
    bank_rank[i].bank = bank_nodes[i].bank; // quedaria BANK_1 (6), BANK_2 (7), BANK_3 (8)
    bank_rank[i].v_mV = bank_nodes[i].v_mV;
  }
}

void Controller_SquareTask(void);
void Controller_mediciones(void);
uint8_t Controller_GetBanksNeeded(uint16_t *achieved_mV);

void Controller_Update(void){

	Controller_mediciones();

	uint16_t tension_lograda = 0;
	if (Controller_GetMode() == CTRL_MODE_AUTO){
		uint8_t cant_bancos = Controller_GetBanksNeeded(&tension_lograda);

		for (uint8_t i = 0; i< cant_bancos; i++){
			for (uint8_t j = 0; j < CELLS_PER_BANK; j++){
			Controller_SetCell(i,j,SW_ON);}
		}
		for (uint8_t i = cant_bancos; i < NUM_BANKS;i ++ ){
			for (uint8_t j = 0; j < CELLS_PER_BANK; j++){
				Controller_SetCell(i,j,SW_OFF);
			}
		}
		tension_controlada = tension_lograda;
	}



}

uint16_t Controller_GetVoltage (void){
	return tension_controlada;
}

uint16_t Tension_medida(void){
	return tension_medida;
}



/*
static uint8_t Controller_MapToModule(uint8_t bank_user,
                                      int8_t cell_user,
                                      module_t *mod)
{
    if (mod == NULL) {
        return 0;
    }

    if (bank_user < 1 || bank_user > NUM_BANKS) {
        return 0;
    }

    // Pedido de banco: (bank, 0)
    if (cell_user == 0) {
        *mod = (module_t)(BANK_1 + (bank_user - 1));
        return 1;
    }

    // Pedido de celda: (bank, cell)
    if (cell_user < 1 || cell_user > CELLS_PER_BANK) {
        return 0;
    }

    *mod = cell_sw[bank_user - 1][cell_user - 1].id;
    return 1;
}*/


/* ======================= CELDAS ======================= */

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




static void Controller_SetCell_ON_(uint8_t bank, uint8_t cell)
{
  cell_cfg[bank][cell].modo = CELL_MODE_STATIC;
  _controller_SetCell_ON(bank,cell);
}

static void Controller_SetCell_OFF_(uint8_t bank, uint8_t cell)
{
  cell_cfg[bank][cell].modo = CELL_MODE_STATIC;
  _controller_SetCell_OFF(bank,cell);
}

void Controller_SetCell(uint8_t bank, uint8_t cell, sw_state_t st)
{
  if (bank >= NUM_BANKS || cell >= CELLS_PER_BANK) return;

  if (st == SW_ON)
	  Controller_SetCell_ON_(bank, cell);
  else
	  Controller_SetCell_OFF_(bank, cell);
}



sw_state_t Controller_GetCell(uint8_t bank, uint8_t cell)
{
  if (bank >= NUM_BANKS || cell >= CELLS_PER_BANK) return SW_OFF;
  return cell_state[bank][cell];
}

uint8_t Controller_GetBankSwitch(uint8_t bank)
{
  if (bank >= NUM_BANKS) return 0;

  return (cell_state[bank][0] == SW_OFF) &&
         (cell_state[bank][1] == SW_OFF);
}


void apagar_celdas(void){
	for (uint8_t i = 0;i<NUM_BANKS;i++){
		for(uint8_t j = 0;j<CELLS_PER_BANK;j++)
		Controller_SetCell(i, j, SW_OFF);
	}
}



uint8_t Controller_SetCellSquare(uint8_t bank,
                                 uint8_t cell,
                                 uint8_t id_grupo,
                                 cell_phase_t fase)
{
  if (bank >= NUM_BANKS || cell >= CELLS_PER_BANK)
    return 0;

  if (id_grupo >= MAX_SQUARE_GROUPS)
    return 0;

  cell_cfg[bank][cell].modo     = CELL_MODE_SIGNAL;
  cell_cfg[bank][cell].id_grupo = id_grupo;
  cell_cfg[bank][cell].fase = fase;

  return 1;
}


uint8_t Controller_ConfigSquareGroup(uint8_t id_grupo, uint16_t freq_hz)
{
  uint32_t ticks;

  if (id_grupo >= MAX_SQUARE_GROUPS || freq_hz == 0)
    return 0;

  ticks = 500U / freq_hz;   // tick base de 1 ms

  if (ticks == 0)
    ticks = 1;

  square_groups[id_grupo].enabled = 1;
  square_groups[id_grupo].ticks = (uint16_t)ticks;
  square_groups[id_grupo].counter = 0;
  square_groups[id_grupo].phase = 0;

  return 1;
}


void Controller_SquareTask(void)
{

	if (Controller_GetMode() == CTRL_MODE_AUTO){
		return;
	}
  uint8_t group_phase_changed[MAX_SQUARE_GROUPS] = {0};
  uint8_t group_phase[MAX_SQUARE_GROUPS] = {0};


  for (uint8_t i = 0; i < MAX_SQUARE_GROUPS; i++) {
    if (!square_groups[i].enabled || square_groups[i].ticks == 0)
      continue;

    square_groups[i].counter++;

    if (square_groups[i].counter >= square_groups[i].ticks) {
      square_groups[i].counter = 0;
      square_groups[i].phase ^= 1U;
      group_phase_changed[i] = 1;
    }

    group_phase[i] = square_groups[i].phase;
  }


  for (uint8_t b = 0; b < NUM_BANKS; b++) {
    for (uint8_t c = 0; c < CELLS_PER_BANK; c++) {
      uint8_t g;


      if (cell_cfg[b][c].modo != CELL_MODE_SIGNAL)
        continue;

      g = cell_cfg[b][c].id_grupo;

      if (g >= MAX_SQUARE_GROUPS)
        continue;

      if (!group_phase_changed[g])
        continue;

      if (cell_cfg[b][c].fase == CELL_PHASE_POS) {
        if (group_phase[g]) {
          _controller_SetCell_ON(b, c);
        } else {
          _controller_SetCell_OFF(b, c);
        }
      } else {
        if (group_phase[g]) {
          _controller_SetCell_OFF(b, c);
        } else {
          _controller_SetCell_ON(b, c);
        }
      }
    }
  }
}

/* ======================= LOGICA DE SELECCION DE BANCO ======================= */

static void _sort_bank_(void)
{

  // Orden descendente por v_mV; desempate por índice menor
  for (uint8_t i = 0; i < NUM_BANKS; i++) {
    for (uint8_t j = i + 1; j < NUM_BANKS; j++) {
      uint8_t swap = 0;

      if (bank_rank[j].v_mV > bank_rank[i].v_mV) swap = 1;
      else if (bank_rank[j].v_mV == bank_rank[i].v_mV &&
               bank_rank[j].bank < bank_rank[i].bank) swap = 1;

      if (swap) {
        bank_v_t tmp = bank_rank[i];
        bank_rank[i] = bank_rank[j];
        bank_rank[j] = tmp;
      }
    }
  }
}

// privado: devuelve top2
/*static uint8_t Controller_get_top2_banks_(uint8_t *b1, uint8_t *b2)
{
  if (NUM_BANKS < 2 || !b1 || !b2) return 0;
  *b1 = bank_rank[0].bank;
  *b2 = bank_rank[1].bank;
  return 1;
}
*/


uint8_t Controller_GetBanksNeeded(uint16_t *achieved_mV)
{
    uint16_t sum_mV = 0;
    uint16_t best_sum_mV = 0;
    uint16_t best_diff = 0xFFFFu;
    uint8_t  best_count = 0;

    for (uint8_t i = 0; i < NUM_BANKS; i++) {
        sum_mV += bank_rank[i].v_mV;

        uint32_t diff;
        if (sum_mV >= g_vtarget_mV) {
            diff = sum_mV - g_vtarget_mV;
        } else {
            diff = g_vtarget_mV - sum_mV;
        }

        if (diff < best_diff) {
            best_diff = diff;
            best_sum_mV = sum_mV;
            best_count = i + 1;
        }
    }

    if (achieved_mV != 0) {
        *achieved_mV = best_sum_mV;
    }

    return best_count;
}






/* =======================  TOMAR MEDICIONES ======================= */



void Controller_cargar_mediciones(void){
	uint16_t aux[ADC_NODE_COUNT];
	adc_get_buffer_voltage(aux,ADC_NODE_COUNT);

    for (uint8_t i = 0; i <= ADC_NODE_CELL_32; i++)
    {
    	cell_nodes[i].v_mV = aux[i];
    }

    for (uint8_t b = 0; b < NUM_BANKS; b++) {
        bank_nodes[b].v_mV = aux[bank_nodes[b].bus_pos] - aux[bank_nodes[b].bus_neg];
    }

    tension_medida = aux[ADC_NODE_OUT_POS]-aux[ADC_NODE_OUT_NEG];

}


static uint16_t abs_diff(uint16_t a, uint16_t b)
{
    return (a > b) ? (a - b) : (b - a);
}



void Controller_mediciones(void){
	Controller_cargar_mediciones();

	/*uint8_t changed = 0;

	for (uint8_t b = 0; b < NUM_BANKS; b++) {
	        if (abs_diff_u16(bank_nodes[b].v_mV, g_bank_voltage_mV[b]) > UMBRAL_mV) {
	            g_bank_voltage_mV[b] = bank_nodes[b].v_mV;
	            changed = 1;
	        }
	    }


	if (changed == 0) return;*/

    for (uint8_t b = 0; b < NUM_BANKS; b++) {
        bank_rank[b] = bank_nodes[b];

    }

    _sort_bank_();

}


uint16_t Controller_getMeasurement(uint8_t bank, uint8_t cell)
{
    if (cell == 0) {
        return bank_nodes[bank - 1].v_mV;
    }

    return cell_nodes[(bank - 1) * CELLS_PER_BANK + (cell - 1)].v_mV;
}


/* ======================= MODO AUTOMATICO (para CLI) ======================= */


/*
 * Setea la tension que debe proporcional el pack de celdas
 */
uint8_t Controller_SetTargetVoltage(float v)
{
  // 3.3..9.9 V -> 3300..9900 mV
  uint16_t v_mV = (uint16_t)(v * 1000.0f + 0.5f);
  if (v_mV < 3300 || v_mV > 9900) return 0;

  g_vtarget_mV = v_mV;
  return 1;
}


uint16_t Controller_GetTargetVoltage(void)
{
  return g_vtarget_mV;
}

uint8_t Controller_SetMode(ctrl_mode_t mode)
{
  //if (mode == CTRL_MODE_AUTO) return 0;
  g_ctrl_mode = mode;
  return 1;
}

ctrl_mode_t Controller_GetMode(void)
{
  return g_ctrl_mode;
}





void Test_ConmutacionConDelay(void)
{
	uint8_t bank = 0;
	uint8_t cell = 1;


	gpio_write(&cell_sw[bank][cell], SW_OFF);
	HAL_Delay(2);
	gpio_write(&bank_sw[bank], SW_ON);

	HAL_Delay(1000);

	gpio_write(&bank_sw[bank], SW_OFF);
	HAL_Delay(2);
	gpio_write(&cell_sw[bank][cell], SW_ON);

	HAL_Delay(1000);

	gpio_write(&cell_sw[bank][cell], SW_OFF);
	HAL_Delay(2);
	gpio_write(&bank_sw[bank], SW_ON);

	HAL_Delay(1000);

	gpio_write(&bank_sw[bank], SW_OFF);
	HAL_Delay(2);
	gpio_write(&cell_sw[bank][cell], SW_ON);
}
