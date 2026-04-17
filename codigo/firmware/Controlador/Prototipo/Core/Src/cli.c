#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "main.h"
#include "cli.h"
#include "controller.h"

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

void str_to_lower(char *s) {
    while (*s) {
        *s = tolower((unsigned char)*s);
        s++;
    }
}


static uint8_t parse_float_strict(const char *s, float *out)
{
  // acepta: "7", "7.4", "7.40"
  // rechaza si queda basura: "7.4V"
  char *end = NULL;
  float v = strtof(s, &end);
  if (s == end) return 0;              // no parseó nada

  while (*end == ' ' || *end == '\t') end++;
  if (*end != '\0') return 0;          // quedó basura

  *out = v;
  return 1;
}

static void print_vtarget(void)
{


  uint16_t vt = Controller_GetTargetVoltage();
  char buf[48];
  snprintf(buf, sizeof(buf),
           "VTARGET: %u.%03u V\r\n",
           (unsigned)(vt / 1000u),
           (unsigned)(vt % 1000u));
  cli_print(buf);
}


static void print_vout(void)
{


  uint16_t vt = Tension_medida();
  char buf[48];
  snprintf(buf, sizeof(buf),
           "TENSION EN LA SALIDA: %u.%03u V\r\n",
           (unsigned)(vt / 1000u),
           (unsigned)(vt % 1000u));
  cli_print(buf);
}


static void print_vcontrolada(void)
{


  uint16_t vt = Controller_GetVoltage();
  char buf[48];
  snprintf(buf, sizeof(buf),
           "TENSION OBJETIVO: %u.%03u V\r\n",
           (unsigned)(vt / 1000u),
           (unsigned)(vt % 1000u));
  cli_print(buf);
}


static void cli_print_status(void)
{
  // Modo + Vtarget
  cli_print(Controller_GetMode() == CTRL_MODE_AUTO ? "MODE=AUTO | " : "MODE=MANUAL | ");
  print_vtarget();
  print_vcontrolada();
  print_vout();

  char buf[120];

  for (uint8_t b = 0; b < 3; b++) {
    uint8_t bank_user = b + 1;

    uint16_t v_cell1_mV = Controller_getMeasurement(bank_user, 1);
    uint16_t v_cell2_mV = Controller_getMeasurement(bank_user, 2);
    uint16_t v_bank_mV  = Controller_getMeasurement(bank_user, 0);

    snprintf(buf, sizeof(buf),
      "BANK %u: C1=%s (%u mV) C2=%s (%u mV) | BANK_SW=%s (%u mV)\r\n",
      (unsigned)bank_user,
      Controller_GetCell(b, 0) ? "ON" : "OFF",
      (unsigned)v_cell1_mV,
      Controller_GetCell(b, 1) ? "ON" : "OFF",
      (unsigned)v_cell2_mV,
      Controller_GetBankSwitch(b) ? "ON" : "OFF",
      (unsigned)v_bank_mV
    );
    cli_print(buf);
  }
}

static void cli_print_help(void)
{
  cli_print(
    "\r\n=== COMMANDS ===\r\n"
    "\r\n"
    "  help\r\n"
    "  status\r\n"
    "\r\n"
    "  vset <3.3..9.9>           Set target voltage\r\n"
    "  vget                      Show target voltage\r\n"
    "\r\n"
    "  modo                      Show control mode\r\n"
    "  modo manual|auto          Set control mode\r\n"
    "\r\n"
    "  cell <b 1-3> <c 1-2> on   Turn cell ON (manual mode)\r\n"
    "  cell <b 1-3> <c 1-2> off  Turn cell OFF / stop switching\r\n"
    "\r\n"
    "  sw <b> <c> <f> <g> <s|c>  Set square signal (manual mode)\r\n"
    "                            b: bank (1-3)\r\n"
    "                            c: cell (1-2)\r\n"
    "                            f: freq (1-30 Hz)\r\n"
    "                            g: group (0-%d)\r\n"
    "                            s: same phase\r\n"
    "                            c: complementary\r\n"
    "\r\n"
    "  Examples:\r\n"
    "    sw 1 1 10 0 s\r\n"
    "    sw 1 2 10 0 s\r\n"
    "    sw 2 1 10 0 c\r\n"
    "\r\n");
}

/* ===================== COMMANDOS ===================== */
static void cli_handle_line(const char *line_in)
{
  // Trim espacios iniciales
  while (*line_in == ' ' || *line_in == '\t') line_in++;
  char line_copy[128];
  strcpy(line_copy, line_in);
  str_to_lower(line_copy);
  if (*line_copy == '\0') return;
   str_to_lower(line_copy);
  // HELP
  if (strcmp(line_copy, "help") == 0) {
    cli_print_help();
    return;
  }

  // STATUS
  if (strcmp(line_copy, "status") == 0) {
    cli_print_status();
    return;
  }

  // VGET
  if (strcmp(line_copy, "vget") == 0) {
    print_vtarget();
    return;
  }


  // VSET <float>
  {
    char cmd[8], arg[24];
    if (sscanf(line_copy, "%7s %23s", cmd, arg) == 2) {
      for (int i = 0; cmd[i]; i++) cmd[i] = (char)tolower((unsigned char)cmd[i]);

      if (strcmp(cmd, "vset") == 0) {
        float v;
        if (!parse_float_strict(arg, &v)) {
          cli_print("ERR: vset <3.3..9.9>\r\n");
          return;
        }

        if (!Controller_SetTargetVoltage(v)) {
          cli_print("ERR: voltage out of range (3.3..9.9)\r\n");
          return;
        }

        cli_print("OK\r\n");
        return;
      }
    }
  }

  // MODE / MODE manual|auto
  {
    char cmd[8], arg[16];
    int n = sscanf(line_copy, "%7s %15s", cmd, arg);
    if (n >= 1) {
      for (int i = 0; cmd[i]; i++) cmd[i] = (char)tolower((unsigned char)cmd[i]);

      if (strcmp(cmd, "modo") == 0) {
        if (n == 1) {
          cli_print(Controller_GetMode() == CTRL_MODE_AUTO ? "MODO: AUTO\r\n" : "MODO: MANUAL\r\n");
          return;
        }

        for (int i = 0; arg[i]; i++) arg[i] = (char)tolower((unsigned char)arg[i]);

        if (strcmp(arg, "manual") == 0) {
          (void)Controller_SetMode(CTRL_MODE_MANUAL);
          cli_print("OK\r\n");
          return;
        }

        if (strcmp(arg, "auto") == 0) {
          if (Controller_SetMode(CTRL_MODE_AUTO)==0) {
            cli_print("ERR: set voltage first with vset (3.3..9.9)\r\n");
            return;
          }
          cli_print("OK\r\n");
          return;
        }

        cli_print("ERR: mode manual|auto\r\n");
        return;
      }
    }
  }

  // CELL <bank 1-3> <cell 1-2> on|off   (manual only)
  {
    unsigned int b_user, c_user;
    char st[8];

    if (sscanf(line_copy, "cell %u %u %7s", &b_user, &c_user, st) == 3) {

      for (int i = 0; st[i]; i++)
        st[i] = (char)tolower((unsigned char)st[i]);

      if (b_user < 1 || b_user > 3 || c_user < 1 || c_user > 2) {
        cli_print("ERR: cell <bank 1-3> <cell 1-2> on|off\r\n");
        return;
      }

      // En AUTO dejamos que el controller maneje las celdas (por ahora)
      if (Controller_GetMode() != CTRL_MODE_MANUAL) {
        cli_print("ERR: cell command allowed only in MANUAL mode\r\n");
        return;
      }

      uint8_t b = (uint8_t)(b_user - 1);
      uint8_t c = (uint8_t)(c_user - 1);

      if (strcmp(st, "on") == 0) {
        Controller_SetCell(b, c, SW_ON);
        cli_print("OK\r\n");
        return;
      }

      if (strcmp(st, "off") == 0) {
        Controller_SetCell(b, c, SW_OFF);
        cli_print("OK\r\n");
        return;
      }

      cli_print("ERR: use on|off\r\n");
      return;
    }
  }
  //switching
  {
	  unsigned int b_user, c_user, freq, grupo;
	  char mode[4];

	  if (sscanf(line_copy, "sw %u %u %u %u %3s",
	             &b_user, &c_user, &freq, &grupo, mode) == 5)
	  {
		  if (b_user < 1 || b_user > NUM_BANKS ||
		      c_user < 1 || c_user > CELLS_PER_BANK) {
		    cli_print("ERR: sw <bank 1-3> <cell 1-2> <freq> <grupo> <s|c>\r\n");
		    return;
		  }

		  if (freq < 1 || freq > 30) {
		    cli_print("ERR: frequency out of range (1-30 Hz)\r\n");
		    return;
		  }

		  if (grupo >= MAX_SQUARE_GROUPS) {
		    cli_print("ERR: invalid group\r\n");
		    return;
		  }
		  cell_phase_t fase;

		  if (strcmp(mode, "s") == 0) {
		    fase = CELL_PHASE_POS;
		  }
		  else if (strcmp(mode, "c") == 0) {
		    fase = CELL_PHASE_NEG;
		  }
		  else {
		  	cli_print("ERR: mode must be s or c\r\n");
		  	return;
		  }

		  uint8_t bank = b_user - 1;
		  uint8_t cell = c_user - 1;

		  Controller_ConfigSquareGroup(grupo, freq);
		  Controller_SetCellSquare(bank, cell, grupo, fase);

		  cli_print("OK\r\n");
		  return;
		  }

  }
  cli_print("ERR: unknown command (try 'help')\r\n");

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
      line_buf[line_len++] = (char)rx_ch;
    }
  }

  HAL_UART_Receive_IT(cli_huart, &rx_ch, 1);
}



void CLI_ButtonManualCallback(uint16_t gpio_pin)
{
  if (gpio_pin != Modo_manual_Pin) return;
  apagar_celdas();
  if (Controller_GetMode() != CTRL_MODE_MANUAL) {
    (void)Controller_SetMode(CTRL_MODE_MANUAL);
    cli_print("\r\nBTN B1: MODE -> MANUAL\r\n> ");
  }
}
