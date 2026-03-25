#include "controller.h"
#include "main.h"
#include <stdint.h>



/* ======================= ESTADO CELDAS ======================= */



static cell_state_t cell_state[NUM_BANKS][CELLS_PER_BANK];



static cell_cfg_t cell_cfg[NUM_BANKS][CELLS_PER_BANK];



typedef struct {
  uint8_t  enabled;
  uint16_t ticks;   // en ticks de 1 ms
  uint16_t counter;
  uint8_t  phase;               // si está en on u off
} square_group_t;

static square_group_t square_groups[MAX_SQUARE_GROUPS];

/* ======================= MAPEOS SWITCHES ======================= */

// Celdas
static pin_switch_t cell_sw[NUM_BANKS][CELLS_PER_BANK] = {
  // Banco 1
  {
    { Vc_11_GPIO_Port, Vc_11_Pin, 1 },
    { Vc_12_GPIO_Port, Vc_12_Pin, 1 }
  },
  // Banco 2
  {
    { Vc_21_GPIO_Port, Vc_21_Pin, 1 },
    { Vc_22_GPIO_Port, Vc_22_Pin, 1 }
  },
  // Banco 3
  {
    { Vc_31_GPIO_Port, Vc_31_Pin, 1 },
    { Vc_32_GPIO_Port, Vc_32_Pin, 1 }
  }
};

// Bancos
static pin_switch_t bank_sw[NUM_BANKS] = {
  { Vc_1_GPIO_Port, Vc_1_Pin, 1 },
  { Vc_2_GPIO_Port, Vc_2_Pin, 1 },
  { Vc_3_GPIO_Port, Vc_3_Pin, 1 }
};

/* ======================= CONFIGURACIONES ======================= */

#define UMBRAL_mV 10  // 10mV (para futuro control)

/* ======================= VARIABLES DE TENSION ======================= */

static ctrl_mode_t g_ctrl_mode = CTRL_MODE_MANUAL;
static uint16_t g_vtarget_mV = 3300;     // mV tension elegida
static uint8_t  g_vtarget_set = 0; 		// booleano, si la tension seteada es valida

/* ======================= MEDICIONES POR BANCO ======================= */

typedef struct {
  uint8_t  bank;   // 0..NUM_BANKS-1
  uint16_t v_mV;   // miliVolt
} bank_v_t;

static uint16_t g_bank_v_mV[NUM_BANKS] = {0};
static bank_v_t g_bank_rank[NUM_BANKS];

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
    (cell_state[bank][0] == CELL_OFF) &&
    (cell_state[bank][1] == CELL_OFF);

  gpio_write(&bank_sw[bank], both_off);
}

/* ======================= INIT ======================= */

void Controller_Init(void)
{
  for (uint8_t b = 0; b < NUM_BANKS; b++) {
    for (uint8_t c = 0; c < CELLS_PER_BANK; c++) {
      cell_state[b][c] = CELL_OFF;

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

  // Inicializar ranking coherente
  for (uint8_t i = 0; i < NUM_BANKS; i++) {
    g_bank_v_mV[i] = 0;
    g_bank_rank[i].bank = i;
    g_bank_rank[i].v_mV = 0;
  }
}

void Controller_SquareTask(void);

void Controller_Update(void){
	Controller_SquareTask();
}

/* ======================= CELDAS ======================= */

static void _controller_SetCell_ON(uint8_t bank, uint8_t cell)
{
  cell_state[bank][cell] = CELL_ON;
  Controller_UpdateBank(bank);
  gpio_write(&cell_sw[bank][cell], 1);
}

static void _controller_SetCell_OFF(uint8_t bank, uint8_t cell)
{
  cell_state[bank][cell] = CELL_OFF;
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

void Controller_SetCell(uint8_t bank, uint8_t cell, cell_state_t st)
{
  if (bank >= NUM_BANKS || cell >= CELLS_PER_BANK) return;

  if (st == CELL_ON)
	  Controller_SetCell_ON_(bank, cell);
  else
	  Controller_SetCell_OFF_(bank, cell);
}



cell_state_t Controller_GetCell(uint8_t bank, uint8_t cell)
{
  if (bank >= NUM_BANKS || cell >= CELLS_PER_BANK) return CELL_OFF;
  return cell_state[bank][cell];
}

uint8_t Controller_GetBankSwitch(uint8_t bank)
{
  if (bank >= NUM_BANKS) return 0;

  return (cell_state[bank][0] == CELL_OFF) &&
         (cell_state[bank][1] == CELL_OFF);
}


void Apagar_celdas(void){
	for (uint8_t i = 0;i<NUM_BANKS;i++){
		for(uint8_t j = 0;j<CELLS_PER_BANK;j++)
		Controller_SetCell(i, j, CELL_OFF);
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

static void Controller_sort_bank_(void)
{
  for (uint8_t i = 0; i < NUM_BANKS; i++) {
    g_bank_rank[i].bank = i;
    g_bank_rank[i].v_mV = g_bank_v_mV[i];
  }

  // Orden descendente por v_mV; desempate por índice menor
  for (uint8_t i = 0; i < NUM_BANKS; i++) {
    for (uint8_t j = i + 1; j < NUM_BANKS; j++) {
      uint8_t swap = 0;

      if (g_bank_rank[j].v_mV > g_bank_rank[i].v_mV) swap = 1;
      else if (g_bank_rank[j].v_mV == g_bank_rank[i].v_mV &&
               g_bank_rank[j].bank < g_bank_rank[i].bank) swap = 1;

      if (swap) {
        bank_v_t tmp = g_bank_rank[i];
        g_bank_rank[i] = g_bank_rank[j];
        g_bank_rank[j] = tmp;
      }
    }
  }
}

// privado: devuelve top2
/*static uint8_t Controller_get_top2_banks_(uint8_t *b1, uint8_t *b2)
{
  if (NUM_BANKS < 2 || !b1 || !b2) return 0;
  *b1 = g_bank_rank[0].bank;
  *b2 = g_bank_rank[1].bank;
  return 1;
}
*/



/*
 * Actualiza los valores de tension de cada banco
 */
void Controller_SetBankVoltages_mV(const uint16_t v_bank_mV[NUM_BANKS])
{
  for (uint8_t i = 0; i < NUM_BANKS; i++) {
    g_bank_v_mV[i] = v_bank_mV[i];
  }
  Controller_sort_bank_();
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
  g_vtarget_set = 1;
  return 1;
}

uint8_t Controller_HasTargetVoltage(void)
{
  return g_vtarget_set; //indica si hay un valor posible elegido
}

uint16_t Controller_GetTargetVoltage(void)
{
  return g_vtarget_set ? g_vtarget_mV : 0;
}

uint8_t Controller_SetMode(ctrl_mode_t mode)
{
  if (mode == CTRL_MODE_AUTO && !g_vtarget_set) return 0;
  g_ctrl_mode = mode;
  return 1;
}

ctrl_mode_t Controller_GetMode(void)
{
  return g_ctrl_mode;
}
