/*
 * cli.c
 *
 *  Created on: 12 jun 2026
 *      Author: ---
 */

/*
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include "main.h"
#include "cli.h"
#include "controlador.h"
*/
/* ===================== CONFIG ===================== */
//#define CLI_BUF_LEN 64

/* ===================== ESTADO ===================== *//*
static UART_HandleTypeDef *cli_huart;
static uint8_t rx_ch;
static char line_buf[CLI_BUF_LEN];
static uint8_t line_len;




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
  char buf[120];

  for (uint8_t b = 0; b < CANT_BANCOS; b++) {
    uint8_t c1_on = Controlador_GetEstadoCelda(b, 0);
    uint8_t c2_on = Controlador_GetEstadoCelda(b, 1);
    uint8_t banco_on = Controlador_GetEstadoBanco(b);

    snprintf(buf, sizeof(buf),
      "BANCO %u: C1=%s | C2=%s | BANCO_SW=%s\r\n",
      (unsigned)b,
      cli_on_off(c1_on),
      cli_on_off(c2_on),
      cli_on_off(banco_on)
    );
    cli_print(buf);
  }
}


static void cli_print_help(void){
	cli_print("\r\n Ayuda en construcción \r\n");
}

*/
/* ===================== MANEJO DE COMANDOS===================== */
/*
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

					if (b_user < 1 || b_user > 4 || c_user < 1 || c_user > 3) {
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

					if (b_user < 1 || b_user > 4 ) {
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
	  {
		  unsigned int b_user, c_user, periodo;
		  char mode[2];
		  if (sscanf(line_copy, "sw %u %u %u %3s",
		  	             &b_user, &c_user, &periodo, mode) == 4){
			  if (b_user < 1 || b_user > CANT_BANCOS ||
			  		      c_user < 1 || c_user > 3) { //harcodeado
			  		    cli_print("ERR: sw <banco 1-4> <celda 1-3> <periodo>  <s|c>\r\n");
			  		    return;
			  }

			  if (periodo < 1 || periodo > 50) { //harcodeado
			  		    cli_print("ERR: periodo fuera de rango (1-50 Hz)\r\n");
			  		    return;
			  }


			  /*if (mode == NULL || mode[1] != '\0') {
			    cli_print("ERROR: modo invalido\r\n");
			    return;
			  }*/
/*

			  uint8_t banco = b_user - 1;
			  uint8_t celda = c_user - 1;

			  Controlador_IniciarSwitchingCelda(banco,celda,mode[0]);
			  Controlador_ModificarPeriodoBanco(banco, periodo);
		  }
	  }


}*/

/* ===================== API ===================== */
/*
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



void CLI_ButtonReiniciarCallback(uint16_t gpio_pin)
{
  if (gpio_pin != Boton_Reinciar_Pin) return;
	Controlador_Reiniciar();
    cli_print("\r\nControlador Reiniciado\r\n> ");
}


*/















