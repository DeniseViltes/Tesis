/*
 * cli.c
 *
 *  Created on: 12 jun 2026
 *      Author: ---
 */


#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include "main.h"
#include "cli.h"
#include "controlador.h"

/* ===================== CONFIG ===================== */
#define CLI_BUF_LEN 64

/* ===================== ESTADO ===================== */
static UART_HandleTypeDef *cli_huart;
static uint8_t rx_ch;
static char line_buf[CLI_BUF_LEN];
static uint8_t line_len;

static volatile uint8_t cli_line_ready = 0;
static char cli_cmd_buf[CLI_BUF_LEN];


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

static void cli_print_voltage(uint16_t voltage_mV)
{
    char buffer[16];

    snprintf(buffer, sizeof(buffer),
             "%u.%03u V\r\n",
             (unsigned int)(voltage_mV / 1000u),
             (unsigned int)(voltage_mV % 1000u));

    cli_print(buffer);
}




static const char *cli_on_off(uint8_t estado)
{
  return estado ? "ON" : "OFF";
}



static void cli_print_status(void)
{
  char buf[160];
  uint8_t celdas[CELDAS_POR_BANCO] = {OFF};

  for (uint8_t b = 0; b < CANT_BANCOS; b++) {

    int len = snprintf(buf, sizeof(buf),
                       "BANCO %u: ",
                       (unsigned)(b + 1));

    for (uint8_t c = 0; c < CELDAS_POR_BANCO; c++) {
      celdas[c] = Controlador_GetEstadoCelda(b, c);

      len += snprintf(&buf[len], sizeof(buf) - len,
                      "C%u=%s%s",
                      (unsigned)(c + 1),
                      cli_on_off(celdas[c]),
                      (c < CELDAS_POR_BANCO - 1) ? " | " : "");
    }

    uint8_t banco_on = Controlador_GetEstadoBanco(b);

    snprintf(&buf[len], sizeof(buf) - len,
             " | BANCO_SW=%s\r\n",
             cli_on_off(banco_on));

    cli_print(buf);
  }
}


static void cli_print_help(void)
{cli_print(
	    "\r\n"
	    "========== AYUDA ==========\r\n"
	    "\r\n"
	    "Comandos generales:\r\n"
	    "  help\r\n"
	    "      Muestra esta ayuda.\r\n"
	    "\r\n"
	    "  status\r\n"
	    "      Muestra el estado de bancos y celdas.\r\n"
	    "\r\n"
	    "Control manual:\r\n"
	    "  c <banco> <celda> on|off\r\n"
	    "      Ejemplo: c 1 3 on\r\n"
	    "\r\n"
	    "  b <banco> on|off\r\n"
	    "      on  : activa todas las celdas.\r\n"
	    "      off : deja el banco en bypass.\r\n"
	    "      Ejemplo: b 1 off\r\n"
	    "\r\n"
	    "Switching:\r\n"
	    "  sw <banco> <modo>\r\n"
	    "      Inicia todo el banco con el periodo actual.\r\n"
	    "      Ejemplo: sw 1 s\r\n"
	    "\r\n"
	    "  sw <banco> <periodo> <modo>\r\n"
	    "      Inicia todo el banco y cambia el periodo global.\r\n"
	    "      Ejemplo: sw 1 500 s\r\n"
	    "\r\n"
	    "  sw <banco> <celda> <modo>\r\n"
	    "      Inicia una celda con el periodo actual.\r\n"
	    "      Ejemplo: sw 1 3 c\r\n"
	    "\r\n"
	    "  sw <banco> <celda> <periodo> <modo>\r\n"
	    "      Inicia una celda y cambia el periodo global.\r\n"
	    "      Ejemplo: sw 1 3 500 s\r\n"
	    "\r\n"
	    "  sw <banco> [<celda>] off\r\n"
	    "      Sin celda: detiene todo el banco.\r\n"
	    "      Con celda: detiene solamente esa celda.\r\n"
	    "      Ejemplos: sw 1 off | sw 1 3 off\r\n"
	    "\r\n"
	    "Modos:\r\n"
	    "  s : sincrono; fase 0 ON, fase 1 OFF.\r\n"
	    "  c : complementario; fase 0 OFF, fase 1 ON.\r\n"
	    "\r\n"
	    "Rangos:\r\n"
	    "  Bancos : 1 a 2\r\n"
	    "  Celdas : 1 a 7\r\n"
	    "  Periodo minimo: 50 ms\r\n"
	    "\r\n"
	    "Bancos intercalados:\r\n"
	    "  sw 1 500 s\r\n"
	    "  sw 2 c\r\n"
	    "\r\n"
	    "El periodo es global y corresponde a una fase.\r\n"
	    "El ciclo completo dura dos periodos.\r\n"
	    "===========================\r\n"
	    "\r\n"
	);}


/* ===================== MANEJO DE COMANDOS===================== */

static void cli_handle_line(const char *line_in){
	while (*line_in == ' ' || *line_in == '\t') line_in++;

	char line_copy[128];
	strcpy(line_copy, line_in);
	str_to_lower(line_copy);
	if (*line_copy == '\0') return;
	//str_to_lower(line_copy);

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


	  /* MUX Y MEDICIONES */
	  {
	      unsigned int b_user;
	      unsigned int c_user;
	      char extra;

	      if (sscanf(line_copy,
	                 "mux %u %u %c",
	                 &b_user,
	                 &c_user,
	                 &extra) == 2)
	      {
	          if (b_user < 1u || b_user > CANT_BANCOS ||
	              c_user < 1u || c_user > CELDAS_POR_BANCO)
	          {
	              cli_print("ERR: mux <banco> <celda>\r\n");
	              return;
	          }

	          uint8_t b = (uint8_t)(b_user - 1u);
	          uint8_t c = (uint8_t)(c_user - 1u);

	          Controlador_SeleccionarCellNeg(b, c);

	          cli_print("MUX seleccionado\r\n");
	          return;
	      }


	      if (sscanf(line_copy,
	                 "medir %u %c",
	                 &b_user,
	                 &extra) == 1)
	      {
	          if (b_user < 1u || b_user > CANT_BANCOS)
	          {
	              cli_print("ERR: medir <banco>\r\n");
	              return;
	          }

	          uint8_t b = (uint8_t)(b_user - 1u);

	          uint16_t medicion_mV =
	              Controlador_MedirCellNeg(b);

	          cli_print("Tension medida: ");
	          cli_print_voltage(medicion_mV);

	          return;
	      }
	  }

	  // CELDA <bank 1-3> <cell 1-2> on|off
	  {
		  unsigned int b_user, c_user;
		  char st[8];

		  if (sscanf(line_copy, "c %u %u %7s", &b_user, &c_user, st) == 3) {
			  for (int i = 0; st[i]; i++)
					  st[i] = (char)tolower((unsigned char)st[i]);

					if (b_user < 1 || b_user > CANT_BANCOS || c_user < 0 || c_user > CELDAS_POR_BANCO) {
					  cli_print("ERR: c <bank 1-3> <cell 1-2> on|off\r\n");
					  return;
					}

					uint8_t b = (uint8_t)(b_user - 1);
					uint8_t c = (uint8_t)(c_user - 1);

					if (strcmp(st, "on") == 0) {
						Controlador_EncenderCelda(b, c);
					  cli_print("OK\r\n");
					  return;
					}

					if (strcmp(st, "off") == 0) {
						Controlador_ApagarCelda(b, c);
					  cli_print("OK\r\n");
					  return;
					}

					cli_print("ERR: use on|off\r\n");
					return;
		  }
	  }
	  //BANCO <bank 1-3>  on|off
	  {
		  unsigned int b_user;
		  char st[8];

		  if (sscanf(line_copy, "b %u  %7s", &b_user, st) == 2) {
			  for (int i = 0; st[i]; i++)
					  st[i] = (char)tolower((unsigned char)st[i]);

					if (b_user < 1 || b_user > CANT_BANCOS ) {
					  cli_print("ERR: b<bank 1-3> on|off\r\n");
					  return;
					}

					uint8_t b = (uint8_t)(b_user - 1);


					if (strcmp(st, "on") == 0) {
						Controlador_ActivarCeldasBanco(b);
					  cli_print("OK\r\n");
					  return;
					}

					if (strcmp(st, "off") == 0) {
						Controlador_BypassBanco(b);
					  cli_print("OK\r\n");
					  return;
					}

					cli_print("ERR: use on|off\r\n");
					return;
		  }
	  }

	  /*
	   * Formatos admitidos:
	   *
	   * sw <banco> <modo>
	   * sw <banco> <periodo> <modo>
	   * sw <banco> <celda> <modo>
	   * sw <banco> <celda> <periodo> <modo>
	   * sw <banco> [<celda>] off
	   */
	  {
	      unsigned int banco_user;
	      unsigned int celda_user;
	      unsigned int valor;
	      unsigned int periodo;

	      uint8_t banco;
	      uint8_t celda;

	      char modo;
	      char extra;


	      /* CASO 1: sw <banco> <celda> <periodo> <modo> */
	      if (sscanf(line_copy,
	                 "sw %u %u %u %c %c",
	                 &banco_user,
	                 &celda_user,
	                 &periodo,
	                 &modo,
	                 &extra) == 4) {

	          if (banco_user < 1 ||
	              banco_user > CANT_BANCOS) {

	              cli_print("ERR: banco fuera de rango\r\n");
	              return;
	          }

	          if (celda_user < 1 ||
	              celda_user > CELDAS_POR_BANCO) {

	              cli_print("ERR: celda fuera de rango\r\n");
	              return;
	          }

	          if (periodo < PERIODO_DEFAULT) {
	              cli_print("ERR: periodo minimo 50 ms\r\n");
	              return;
	          }

	          if (modo != 's' && modo != 'c') {
	              cli_print("ERR: modo invalido (use s o c)\r\n");
	              return;
	          }

	          banco = (uint8_t)(banco_user - 1);
	          celda = (uint8_t)(celda_user - 1);

	          Controlador_ModificarPeriodo(
	              (uint16_t)periodo
	          );

	          Controlador_IniciarSwitchingCelda(
	              banco,
	              celda
	          );

	          Controlador_ModificarModoCelda(
	              banco,
	              celda,
	              modo
	          );

	          cli_print("OK\r\n");
	          return;
	      }


	      /* CASO 2: sw <banco> <celda> <modo> o sw <banco> <periodo> <modo> */
	      if (sscanf(line_copy,
	                 "sw %u %u %c %c",
	                 &banco_user,
	                 &valor,
	                 &modo,
	                 &extra) == 3) {

	          if (banco_user < 1 ||
	              banco_user > CANT_BANCOS) {

	              cli_print("ERR: banco fuera de rango\r\n");
	              return;
	          }

	          if (modo != 's' && modo != 'c') {
	              cli_print("ERR: modo invalido (use s o c)\r\n");
	              return;
	          }

	          banco = (uint8_t)(banco_user - 1);

	          /* CASO 2A: sw <banco> <celda> <modo> */
	          if (valor >= 1 &&
	              valor <= CELDAS_POR_BANCO) {

	              celda = (uint8_t)(valor - 1);

	              Controlador_IniciarSwitchingCelda(
	                  banco,
	                  celda
	              );

	              Controlador_ModificarModoCelda(
	                  banco,
	                  celda,
	                  modo
	              );

	              cli_print("OK\r\n");
	              return;
	          }

	          /* CASO 2B: sw <banco> <periodo> <modo> */
	          if (valor >= PERIODO_DEFAULT) {

	              Controlador_ModificarPeriodo(
	                  (uint16_t)valor
	              );

	              Controlador_IniciarSwitchingBanco(
	                  banco
	              );

	              Controlador_ModificarModo(
	                  banco,
	                  modo
	              );

	              cli_print("OK\r\n");
	              return;
	          }

	          cli_print(
	              "ERR: celda invalida o periodo menor a 50 ms\r\n"
	          );
	          return;
	      }


	      /* CASO 3: sw <banco> <celda> off */
	      if (sscanf(line_copy,
	                 "sw %u %u off %c",
	                 &banco_user,
	                 &celda_user,
	                 &extra) == 2) {

	          if (banco_user < 1 ||
	              banco_user > CANT_BANCOS) {

	              cli_print("ERR: banco fuera de rango\r\n");
	              return;
	          }

	          if (celda_user < 1 ||
	              celda_user > CELDAS_POR_BANCO) {

	              cli_print("ERR: celda fuera de rango\r\n");
	              return;
	          }

	          banco = (uint8_t)(banco_user - 1);
	          celda = (uint8_t)(celda_user - 1);

	          Controlador_PararSwitchingCelda(
	              banco,
	              celda
	          );

	          cli_print("OK\r\n");
	          return;
	      }


	      /* CASO 4: sw <banco> <modo> */
	      if (sscanf(line_copy,
	                 "sw %u %c %c",
	                 &banco_user,
	                 &modo,
	                 &extra) == 2) {

	          if (banco_user < 1 ||
	              banco_user > CANT_BANCOS) {

	              cli_print("ERR: banco fuera de rango\r\n");
	              return;
	          }

	          if (modo != 's' && modo != 'c') {
	              cli_print("ERR: modo invalido (use s o c)\r\n");
	              return;
	          }

	          banco = (uint8_t)(banco_user - 1);

	          Controlador_IniciarSwitchingBanco(
	              banco
	          );

	          Controlador_ModificarModo(
	              banco,
	              modo
	          );

	          cli_print("OK\r\n");
	          return;
	      }


	      /* CASO 5: sw <banco> off */
	      if (sscanf(line_copy,
	                 "sw %u off %c",
	                 &banco_user,
	                 &extra) == 1) {

	          if (banco_user < 1 ||
	              banco_user > CANT_BANCOS) {

	              cli_print("ERR: banco fuera de rango\r\n");
	              return;
	          }

	          banco = (uint8_t)(banco_user - 1);

	          Controlador_DetenerSwitchingBancoBypass(
	              banco
	          );

	          cli_print("OK\r\n");
	          return;
	      }
	  }
	  cli_print(
	      "ERR: comando invalido. Use help\r\n"
	  );

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
	HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
	if (rx_ch == '\r' || rx_ch == '\n') {
	  if (line_len > 0) {
	    line_buf[line_len] = '\0';
	    strcpy(cli_cmd_buf, line_buf);
	    line_len = 0;
	    cli_line_ready = 1;
	  }
	} else {
	  if (line_len < CLI_BUF_LEN - 1) {
	    line_buf[line_len++] = (char)rx_ch;
	  }
	}

	HAL_UART_Receive_IT(cli_huart, &rx_ch, 1);
}



void CLI_ButtonReiniciarCallback(uint16_t gpio_pin)
{
  if (gpio_pin != Boton_Reinciar_Pin) return;
	Controlador_Reiniciar();
    cli_print("\r\nControlador Reiniciado\r\n> ");
}


void CLI_Process(void)
{
    if (!cli_line_ready)
    {
        return;
    }

    cli_line_ready = 0u;

    cli_print("Procesando: [");
    cli_print(cli_cmd_buf);
    cli_print("]\r\n");

    cli_handle_line(cli_cmd_buf);
    cli_print("> ");
}














