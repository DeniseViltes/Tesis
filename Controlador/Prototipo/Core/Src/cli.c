#include "cli.h"
#include "controller.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

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
  HAL_UART_Transmit(cli_huart, (uint8_t *)s, (uint16_t)strlen(s), HAL_MAX_DELAY);
}

static void cli_print_status(void)
{
  char buf[80];

  for (uint8_t b = 0; b < 3; b++) {
    // mostrar numeración humana: banco 1..3, celda 1..2
    snprintf(buf, sizeof(buf),
      "BANK %u: C1=%s C2=%s | BANK_SW=%s\r\n",
      (unsigned)(b + 1),
      Controller_GetCell(b, 0) ? "ON" : "OFF",
      Controller_GetCell(b, 1) ? "ON" : "OFF",
      Controller_GetBankSwitch(b) ? "ON" : "OFF"
    );
    cli_print(buf);
  }
}

static void cli_print_help(void)
{
  cli_print(
    "Commands:\r\n"
    "  cell <bank 1-3> <cell 1-2> on|off\r\n"
    "  status\r\n"
    "  help\r\n"
  );
}

/* ===================== COMMANDOS ===================== */
static void cli_handle_line(const char *line)
{
  // Ignorar líneas vacías / espacios
  while (*line == ' ' || *line == '\t') line++;
  if (*line == '\0') return;

  if (strcmp(line, "help") == 0) {
    cli_print_help();
    return;
  }

  if (strcmp(line, "status") == 0) {
    cli_print_status();
    return;
  }

  // cell <bank 1-3> <cell 1-2> on|off
  unsigned int b_user, c_user;
  char st[8];

  if (sscanf(line, "cell %u %u %7s", &b_user, &c_user, st) == 3) {

    for (int i = 0; st[i]; i++)
      st[i] = (char)tolower((unsigned char)st[i]);

    if (b_user < 1 || b_user > 3 || c_user < 1 || c_user > 2) {
      cli_print("ERR: cell <bank 1-3> <cell 1-2> on|off\r\n");
      return;
    }

    uint8_t b = (uint8_t)(b_user - 1);
    uint8_t c = (uint8_t)(c_user - 1);

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

    cli_print("ERR: use on|off\r\n");
    return;
  }

  // Si no matcheó nada:
  cli_print("ERR: unknown command (try 'help')\r\n");
}


/*
 * Funcion que seleccione la tensión que requiere el usuario
 * Funcion que seleccione el funcionamiento, por cli o por controlador de tensiones
 * ver si puede hacer que se cambien en cualquier momento ambas formas sin problemas de funcionamiento
 */



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
      line_buf[line_len++] = (char)rx_ch;
    }
  }

  HAL_UART_Receive_IT(cli_huart, &rx_ch, 1);
}
