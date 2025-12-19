#include "controller.h"
#include <stdio.h>
#include <string.h>

static cell_state_t cell_state[NUM_BANKS][CELLS_PER_BANK];


static gpio_out_t cell_sw[NUM_BANKS][CELLS_PER_BANK] = {
  { {Vc_11_GPIO_Port, Vc_11_Pin, 1}, {Vc_12_GPIO_Port, Vc_12_Pin, 1} },
  { {Vc_21_GPIO_Port, Vc_21_Pin, 1}, {Vc_22_GPIO_Port, Vc_22_Pin, 1} },
  { {Vc_31_GPIO_Port, Vc_31_Pin, 1}, {Vc_32_GPIO_Port, Vc_32_Pin, 1} },
};

static gpio_out_t bank_sw[NUM_BANKS] = {
  {Vc_1_GPIO_Port, Vc_1_Pin, 1},
  {Vc_2_GPIO_Port, Vc_2_Pin, 1},
  {Vc_3_GPIO_Port, Vc_3_Pin, 1},
};

static void gpio_write(const gpio_out_t* g, uint8_t on)
{
  GPIO_PinState ps;
  if (g->active_high) ps = on ? GPIO_PIN_SET : GPIO_PIN_RESET;
  else               ps = on ? GPIO_PIN_RESET : GPIO_PIN_SET;
  HAL_GPIO_WritePin(g->port, g->pin, ps);
}

static void Controller_UpdateBank(uint8_t b)
{
  // Regla: banco ON si ambas celdas OFF
  uint8_t both_off = (cell_state[b][0] == CELL_OFF) && (cell_state[b][1] == CELL_OFF);
  gpio_write(&bank_sw[b], both_off ? 1 : 0);
}

void Controller_Init(void)
{
  // estado inicial: todo OFF
  for (uint8_t b = 0; b < NUM_BANKS; b++) {
    for (uint8_t c = 0; c < CELLS_PER_BANK; c++) {
      cell_state[b][c] = CELL_OFF;
      gpio_write(&cell_sw[b][c], 0);
    }
    Controller_UpdateBank(b);
  }
}

void Controller_SetCell(uint8_t bank, uint8_t cell, cell_state_t st)
{
  if (bank >= NUM_BANKS || cell >= CELLS_PER_BANK) return;

  cell_state[bank][cell] = st;
  gpio_write(&cell_sw[bank][cell], (st == CELL_ON) ? 1 : 0);
  Controller_UpdateBank(bank);
}

cell_state_t Controller_GetCell(uint8_t bank, uint8_t cell)
{
  if (bank >= NUM_BANKS || cell >= CELLS_PER_BANK) return CELL_OFF;
  return cell_state[bank][cell];
}

uint8_t Controller_GetBankSwitch(uint8_t bank)
{
  if (bank >= NUM_BANKS) return 0;
  return (cell_state[bank][0] == CELL_OFF) && (cell_state[bank][1] == CELL_OFF);
}

static void uart_print(UART_HandleTypeDef* huart, const char* s)
{
  HAL_UART_Transmit(huart, (uint8_t*)s, (uint16_t)strlen(s), HAL_MAX_DELAY);
}

void Controller_PrintStatus(UART_HandleTypeDef* huart)
{
  char buf[128];
  for (uint8_t b = 0; b < NUM_BANKS; b++) {
    snprintf(buf, sizeof(buf),
      "BANK %u: C0=%s C1=%s | BANK_SW=%s\r\n",
      b,
      cell_state[b][0] ? "ON" : "OFF",
      cell_state[b][1] ? "ON" : "OFF",
      Controller_GetBankSwitch(b) ? "ON" : "OFF"
    );
    uart_print(huart, buf);
  }
}
