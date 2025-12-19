#include "cli.h"
#include "controller.h"
#include <string.h>
#include <stdio.h>

/* ===================== CONFIG ===================== */
#define CLI_BUF_LEN 64

/* ===================== ESTADO ===================== */
static UART_HandleTypeDef *cli_huart;
static uint8_t rx_ch;
static char line_buf[CLI_BUF_LEN];
static uint8_t line_len;

/* ===================== UTILS ===================== */
static void cli_print(const char *s)
{
  HAL_UART_Transmit(cli_huart, (uint8_t *)s, strlen(s), HAL_MAX_DELAY);
}

/* ===================== COMMANDOS ===================== */
static void cli_handle_line(const char *line)
{
  uint8_t b, c;
  char st[8];

  if (strcmp(line, "help") == 0) {
    cli_print(
      "Commands:\r\n"
      "  cell <bank 0-2> <cell 0-1> on|off\r\n"
      "  status\r\n"
      "  help\r\n"
    );
    return;
  }

  if (strcmp(line, "status") == 0) {
    char buf[64];
    for (uint8_t i = 0; i < 3; i++) {
      snprintf(buf, sizeof(buf),
        "BANK %u: C0=%s C1=%s | BANK=%s\r\n",
        i,
        Controller_GetCell(i,0) ? "ON" : "OFF",
        Controller_GetCell(i,1) ? "ON" : "OFF",
        Controller_GetBankSwitch(i) ? "ON" : "OFF"
      );
      cli_print(buf);
    }
    return;
  }

  if (sscanf(line, "cell %hhu %hhu %7s", &b, &c, st) == 3) {
    if (strcmp(st, "on") == 0) {
      Controller_SetCell(b, c, CELL_ON);
      cli_print("OK\r\n");
      return;
    }
    if (strcmp(st, "off") == 0) {
      Controller_SetCell(b, c, CELL_OFF);
      cli_print("OK\r\n");
      return;
    }
  }

  cli_print("ERR\r\n");
}

/* ===================== API ===================== */
void CLI_Init(UART_HandleTypeDef *huart)
{
  cli_huart = huart;
  line_len = 0;

  cli_print("\r\nCell Controller ready\r\n> ");
  HAL_UART_Receive_IT(cli_huart, &rx_ch, 1);
}

void CLI_RxCallback(UART_HandleTypeDef *huart)
{
  if (huart != cli_huart) return;

  if (rx_ch == '\r' || rx_ch == '\n') {
    if (line_len > 0) {
      line_buf[line_len] = '\0';
      cli_handle_line(line_buf);
      line_len = 0;
    }
    cli_print("> ");
  } else {
    if (line_len < CLI_BUF_LEN - 1) {
      line_buf[line_len++] = rx_ch;
    }
  }

  HAL_UART_Receive_IT(cli_huart, &rx_ch, 1);
}
