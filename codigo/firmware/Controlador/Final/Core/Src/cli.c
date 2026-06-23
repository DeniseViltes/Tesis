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


static void cli_print_help(void){
	cli_print("\r\n Ayuda en construcción \r\n");
}


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

	  // CELDA <bank 1-3> <cell 1-2> on|off
	  {
		  unsigned int b_user, c_user;
		  char st[8];

		  if (sscanf(line_copy, "celda %u %u %7s", &b_user, &c_user, st) == 3) {
			  for (int i = 0; st[i]; i++)
					  st[i] = (char)tolower((unsigned char)st[i]);

					if (b_user < 1 || b_user > CANT_BANCOS || c_user < 0 || c_user > CELDAS_POR_BANCO) {
					  cli_print("ERR: celda <bank 1-3> <cell 1-2> on|off\r\n");
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

		  if (sscanf(line_copy, "banco %u  %7s", &b_user, st) == 2) {
			  for (int i = 0; st[i]; i++)
					  st[i] = (char)tolower((unsigned char)st[i]);

					if (b_user < 1 || b_user > CANT_BANCOS ) {
					  cli_print("ERR: banco <bank 1-3> on|off\r\n");
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

	  //switching
	  /*
	   * sw <banco>                          // inicia banco
	   *sw <banco> <periodo>                // inicia banco y cambia período
	   *sw <banco> <celda>                  // inicia celda
	   *sw <banco> <celda> <modo>           // inicia/cambia modo
	   *sw <banco> <celda> <periodo>        // inicia y cambia período
	   *sw <banco> <celda> <periodo> <modo>
	   */

	  {
		  unsigned int banco_user;
		  unsigned int valor;
		  unsigned int periodo;
		  char modo;
		  char extra;

		  uint8_t banco;
		  uint8_t celda;

		  /* sw <banco> <celda> <periodo> <modo> */
		  if (sscanf(line_copy, "sw %u %u %u %c %c",
					 &banco_user, &valor, &periodo, &modo, &extra) == 4) {

			  if (banco_user < 1 || banco_user > CANT_BANCOS) {
				  cli_print("ERR: banco fuera de rango\r\n");
				  return;
			  }

			  if (valor < 1 || valor > CELDAS_POR_BANCO) {
				  cli_print("ERR: celda fuera de rango (1-3)\r\n");
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
			  celda = (uint8_t)(valor - 1);

			  Controlador_ModificarPeriodoBanco(banco, periodo);
			  Controlador_ModificarModoCelda(banco,celda, modo);
			  Controlador_IniciarSwitchingCelda(banco, celda);

			  cli_print("OK\r\n");
			  return;
		  }

		  /* sw <banco> <celda> <periodo> */
		  if (sscanf(line_copy, "sw %u %u %u %c",
					 &banco_user, &valor, &periodo, &extra) == 3) {

			  if (banco_user < 1 || banco_user > CANT_BANCOS) {
				  cli_print("ERR: banco fuera de rango\r\n");
				  return;
			  }

			  if (valor < 1 || valor > CELDAS_POR_BANCO) {
				  cli_print("ERR: celda fuera de rango (1-3)\r\n");
				  return;
			  }

			  if (periodo < PERIODO_DEFAULT) {
				  cli_print("ERR: periodo minimo 50 ms\r\n");
				  return;
			  }

			  banco = (uint8_t)(banco_user - 1);
			  celda = (uint8_t)(valor - 1);

			  Controlador_ModificarPeriodoBanco(banco, periodo);

			  /*
			   * Si la celda conserva su modo actual, conviene tener
			   * una función de inicio que no reciba el modo.
			   */
			  Controlador_IniciarSwitchingCelda(banco, celda);

			  cli_print("OK\r\n");
			  return;
		  }

		  /* sw <banco> <celda> <modo> */
		  if (sscanf(line_copy, "sw %u %u %c %c",
					 &banco_user, &valor, &modo, &extra) == 3) {

			  if (banco_user < 1 || banco_user > CANT_BANCOS) {
				  cli_print("ERR: banco fuera de rango\r\n");
				  return;
			  }

			  if (valor < 1 || valor > 3) {
				  cli_print("ERR: celda fuera de rango (1-3)\r\n");
				  return;
			  }

			  if (modo != 's' && modo != 'c') {
				  cli_print("ERR: modo invalido (use s o c)\r\n");
				  return;
			  }

			  banco = (uint8_t)(banco_user - 1);
			  celda = (uint8_t)(valor - 1);

			  Controlador_ModificarModoCelda(banco, celda, modo);
			  Controlador_IniciarSwitchingCelda(banco, celda);

			  cli_print("OK\r\n");
			  return;
		  }

		  /*
		   * sw <banco> <celda>
		   * sw <banco> <periodo>
		   *
		   * Los valores 1-3 representan una celda.
		   * Los valores >= 50 representan un período.
		   */
		  if (sscanf(line_copy, "sw %u %u %c",
					 &banco_user, &valor, &extra) == 2) {

			  if (banco_user < 1 || banco_user > CANT_BANCOS) {
				  cli_print("ERR: banco fuera de rango\r\n");
				  return;
			  }

			  banco = (uint8_t)(banco_user - 1);

			  if (valor >= 1 && valor <= CELDAS_POR_BANCO) {
				  celda = (uint8_t)(valor - 1);

				  Controlador_IniciarSwitchingCelda(banco, celda);

				  cli_print("OK\r\n");
				  return;
			  }

			  if (valor >= PERIODO_DEFAULT) {
				  Controlador_ModificarPeriodoBanco(banco, valor);
				  Controlador_IniciarSwitchingBanco(banco);

				  cli_print("OK\r\n");
				  return;
			  }

			  cli_print("ERR: celda invalida o periodo menor a 50 ms\r\n");
			  return;
		  }

		  /* sw <banco> */
		  if (sscanf(line_copy, "sw %u %c",
					 &banco_user, &extra) == 1) {

			  if (banco_user < 1 || banco_user > CANT_BANCOS) {
				  cli_print("ERR: banco fuera de rango\r\n");
				  return;
			  }

			  banco = (uint8_t)(banco_user - 1);

			  Controlador_IniciarSwitchingBanco(banco);

			  cli_print("OK\r\n");
			  return;
		  }

		  /* sw <banco> off*/
		  		  if (sscanf(line_copy, "sw %u off %c",
		  					 &banco_user, &extra) == 1) {

		  			  if (banco_user < 1 || banco_user > CANT_BANCOS) {
		  				  cli_print("ERR: banco fuera de rango\r\n");
		  				  return;
		  			  }

		  			  banco = (uint8_t)(banco_user - 1);

		  			Controlador_DetenerSwitchingBancoBypass(banco);

		  			  cli_print("OK\r\n");
		  			  return;
		  		  }

		/* sw <banco> <celda> off*/
				  if (sscanf(line_copy, "sw %u %u off %c",
							 &banco_user, &valor, &extra) == 2) {

					  if (banco_user < 1 || banco_user > CANT_BANCOS) {
						  cli_print("ERR: banco fuera de rango\r\n");
						  return;
					  }
					  if (valor < 1 || valor > 3) {
									  cli_print("ERR: celda fuera de rango (1-3)\r\n");
									  return;

					  banco = (uint8_t)(banco_user - 1);
					  celda = (uint8_t)(valor - 1);

					  Controlador_PararSwitchingCelda(banco,celda);

					  cli_print("OK\r\n");
					  return;
				  }
			  }


	  }

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
  if (!cli_line_ready) return;

  cli_line_ready = 0;
  cli_handle_line(cli_cmd_buf);
  cli_print("> ");
}















