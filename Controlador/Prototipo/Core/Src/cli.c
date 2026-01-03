#include "cli.h"
#include "controller.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

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
  if (!Controller_HasTargetVoltage()) {
    cli_print("VTARGET: NOT_SET\r\n");
    return;
  }

  uint16_t vt = Controller_GetTargetVoltage_x100();
  char buf[48];
  snprintf(buf, sizeof(buf),
           "VTARGET: %u.%02u V\r\n",
           (unsigned)(vt / 100),
           (unsigned)(vt % 100));
  cli_print(buf);
}

static void cli_print_status(void)
{
  // Modo + Vtarget
  cli_print(Controller_GetMode() == CTRL_MODE_AUTO ? "MODE=AUTO | " : "MODE=MANUAL | ");
  print_vtarget();

  // Estado de bancos/celdas
  char buf[80];
  for (uint8_t b = 0; b < 3; b++) {
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
    "  help\r\n"
    "  status\r\n"
    "  vset <3.3..9.9>                 set target voltage\r\n"
    "  vget                            show target voltage\r\n"
    "  mode                            show control mode\r\n"
    "  mode manual|auto                set control mode (auto needs vset)\r\n"
    "  cell <bank 1-3> <cell 1-2> on|off   (manual only)\r\n"
  );
}

/* ===================== COMMANDOS ===================== */
static void cli_handle_line(const char *line_in)
{
  // Trim espacios iniciales
  while (*line_in == ' ' || *line_in == '\t') line_in++;
  if (*line_in == '\0') return;

  // HELP
  if (strcmp(line_in, "help") == 0) {
    cli_print_help();
    return;
  }

  // STATUS
  if (strcmp(line_in, "status") == 0) {
    cli_print_status();
    return;
  }

  // VGET
  if (strcmp(line_in, "vget") == 0) {
    print_vtarget();
    return;
  }

  // VSET <float>
  {
    char cmd[8], arg[24];
    if (sscanf(line_in, "%7s %23s", cmd, arg) == 2) {
      for (int i = 0; cmd[i]; i++) cmd[i] = (char)tolower((unsigned char)cmd[i]);

      if (strcmp(cmd, "vset") == 0) {
        float v;
        if (!parse_float_strict(arg, &v)) {
          cli_print("ERR: vset <3.3..9.9>\r\n");
          return;
        }

        if (v < 3.3f || v > 9.9f) {
          cli_print("ERR: voltage out of range (3.3..9.9)\r\n");
          return;
        }

        // Convertir a x100
        uint16_t v_x100 = (uint16_t)(v * 100.0f + 0.5f);

        if (!Controller_SetTargetVoltage_x100(v_x100)) {
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
    int n = sscanf(line_in, "%7s %15s", cmd, arg);
    if (n >= 1) {
      for (int i = 0; cmd[i]; i++) cmd[i] = (char)tolower((unsigned char)cmd[i]);

      if (strcmp(cmd, "mode") == 0) {
        if (n == 1) {
          cli_print(Controller_GetMode() == CTRL_MODE_AUTO ? "MODE: AUTO\r\n" : "MODE: MANUAL\r\n");
          return;
        }

        for (int i = 0; arg[i]; i++) arg[i] = (char)tolower((unsigned char)arg[i]);

        if (strcmp(arg, "manual") == 0) {
          (void)Controller_SetMode(CTRL_MODE_MANUAL);
          cli_print("OK\r\n");
          return;
        }

        if (strcmp(arg, "auto") == 0) {
          if (!Controller_SetMode(CTRL_MODE_AUTO)) {
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

    if (sscanf(line_in, "cell %u %u %7s", &b_user, &c_user, st) == 3) {

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
