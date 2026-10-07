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

  for (uint8_t b = 0; b < Controlador_GetCantBancos(); b++) {

    int len = snprintf(buf, sizeof(buf),
                       "BANCO %u: ",
                       (unsigned)(b + 1));

    for (uint8_t c = 0; c < Controlador_GetCantCeldas(); c++) {
      celdas[c] = Controlador_GetEstadoCelda(b, c);

      len += snprintf(&buf[len], sizeof(buf) - len,
                      "C%u=%s%s",
                      (unsigned)(c + 1),
                      cli_on_off(celdas[c]),
                      (c + 1u < Controlador_GetCantCeldas()) ? " | " : "");
    }

    uint8_t banco_on = Controlador_GetEstadoBanco(b);

    snprintf(&buf[len], sizeof(buf) - len,
             " | BANCO_SW=%s\r\n",
             cli_on_off(banco_on));

    cli_print(buf);
  }
}





void cli_print_mediciones(void)
{
    char mensaje[96];

    if (!adc_has_measurements()) {
        cli_print("El ADC todavía no recopilo suficientes datos\r\n");
        return;
    }

    snprintf(mensaje, sizeof(mensaje), "Tensión en la salida: %u mV\r\n",
                 (unsigned)Controlador_GetTensionSalida());
    cli_print(mensaje);
    for (uint8_t b = 0; b < Controlador_GetCantBancos(); b++) {
        snprintf(mensaje, sizeof(mensaje),
                 "Banco %u: %u mV \r\n",
                 (unsigned)(b + 1u),
                 (unsigned)Controlador_GetTensionBanco(b));
        cli_print(mensaje);
    }

    snprintf(mensaje, sizeof(mensaje), "Corriente de descarga: %u mA\r\n",
             (unsigned)Controlador_GetCorrienteDescarga());
    cli_print(mensaje);
    snprintf(mensaje, sizeof(mensaje), "Corriente de carga: %u mA\r\n",
             (unsigned)Controlador_GetCorrienteCarga());
    cli_print(mensaje);
}

static void cli_print_help(void)
{cli_print(
	    "\r\n"
	    "========== AYUDA ==========\r\n"
	    "\r\n"
	    "Comandos generales:\r\n"
        "  init <bancos 1..4> <celdas 1..7>\r\n"
        "      Configura celdas por banco, habilita Vcc y activa bypass.\r\n"
        "      Detiene switching y borra RL. Ejemplo: init 2 3\r\n"
		"	   Reinicia el circuito \r\n"
		"  activar carga\r\n"
		"      Habilita el circuito para cargar la bateria.\r\n"
		"  activar descarga <RL_ohm>\r\n"
		"      RL obligatoria: 1..65535 ohm enteros. Ejemplo: activar descarga 10\r\n"
		"  prueba\r\n"
		"      Habilita switches sin chequeos electricos.\r\n"
		"  Para cambiar de modo, ejecutar init nuevamente.\r\n"
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
	    "  Bancos : 1 hasta la cantidad configurada\r\n"
	    "  Celdas : 1 hasta la cantidad configurada\r\n"
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


	// HELP
	if (strcmp(line_copy, "help") == 0) {
	    cli_print_help();
	    return;
	  }


	unsigned bancos, celdas;
	char extra;

	if (sscanf(line_copy, "init%1u%1u%c",
	           &bancos, &celdas, &extra) == 2)
	{
	    if (bancos < 1 || bancos > 4 ||
	        celdas < 1 || celdas > 7)
	    {
	        cli_print("ERR: configuracion fuera de rango\r\n");
	        return;
	    }

	    if (Controlador_init((uint8_t)bancos,
	                         (uint8_t)celdas))
	    {
	        cli_print("ERR: initXY\r\n");
	        return;
	    }

	    cli_print("OK: matriz configurada\r\n");
	    cli_print_status();
	    return;
	}



    if (Controlador_GetCantBancos() == 0) {
        cli_print("ERR: primero use init <bancos> <celdas>\r\n");
        return;
    }

    //ACTIVAR Y  PRUEBA

    if (strncmp(line_copy, "activar", 7) == 0) {
        char modo[16];
        char extra;
        unsigned int rl;

        if (sscanf(line_copy, "activar %15s %u %c",
                   modo, &rl, &extra) == 2 &&
            strcmp(modo, "descarga") == 0) {

            if (rl == 0 || rl > UINT16_MAX) {
                cli_print("ERR: RL debe ser 1..65535 ohm\r\n");
                return;
            }

            if (Controlador_SetResistenciaRL((uint16_t)rl) != 0 ||
                Controlador_ActivarCircuito(MODO_DESCARGA) != 0) {
                cli_print("ERR: no se pudo activar descarga\r\n");
                return;
            }

            cli_print("OK: circuito activo en DESCARGA\r\n");
            cli_print_status();
            return;
        }

        if (strcmp(line_copy, "activar carga") == 0) {
            if (Controlador_ActivarCircuito(MODO_CARGA) != 0) {
                cli_print("ERR: activar requiere estado REPOSO\r\n");
                return;
            }

            cli_print("OK: circuito activo en CARGA\r\n");
            cli_print_status();
            return;
        }

        cli_print("ERR: activar carga | activar descarga <RL_ohm>\r\n");
        return;
    }

    if (strcmp(line_copy, "prueba") == 0) {
        if (Controlador_ModoPrueba() != 0) {
            cli_print("ERR: prueba requiere estado REPOSO\r\n");
            return;
        }

        cli_print("OK: modo PRUEBA, sin chequeos electricos\r\n");
        cli_print_status();
        return;
    }

	// STATUS
	  if (strcmp(line_copy, "status") == 0) {
	    cli_print_status();
	    return;
	  }


	  /* MUX Y MEDICIONES */
	  {
	      unsigned int c_user;
	      char extra;

	      if (sscanf(line_copy,
	                 "mux %u  %c",
	                 &c_user,
	                 &extra) == 1)
	      {
	          if (c_user < 1u || c_user > Controlador_GetCantCeldas())
	          {
	              cli_print("ERR: mux <celda configurada>\r\n");
	              return;
	          }


	          /* CLI: celdas 1..7; MUX_Select: indices 0..6. */
	          uint8_t c = (uint8_t)(c_user - 1u);

	          if (Controlador_SeleccionarMux(c) == 1){
	        	  cli_print("ERR: activar la plataforma\r\n");
	          }

	          cli_print("MUX seleccionado\r\n");
	          return;
	      }


	      if (strcmp(line_copy, "medir") == 0)
	      {

		  Controlador_CargarMediciones();
		  cli_print_mediciones();



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

					if (b_user < 1 || b_user > Controlador_GetCantBancos() || c_user < 1 || c_user > Controlador_GetCantCeldas()) {
					  cli_print("ERR: c <banco configurado> <celda configurada> on|off\r\n");
					  return;
					}

					uint8_t b = (uint8_t)(b_user - 1);
					uint8_t c = (uint8_t)(c_user - 1);

					if (strcmp(st, "on") == 0) {
						if (Controlador_EncenderCelda(b, c) == 1) {
                  cli_print("ERR: operacion no permitida o parametros invalidos\r\n");
                  return;
              }
					  cli_print("OK\r\n");
					  return;
					}

					if (strcmp(st, "off") == 0) {
						if (Controlador_ApagarCelda(b, c) == 1) {
                  cli_print("ERR: operacion no permitida o parametros invalidos\r\n");
                  return;
              }
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

					if (b_user < 1 || b_user > Controlador_GetCantBancos() ) {
					  cli_print("ERR: b <banco configurado> on|off\r\n");
					  return;
					}

					uint8_t b = (uint8_t)(b_user - 1);


					if (strcmp(st, "on") == 0) {
						if (Controlador_ActivarCeldasBanco(b) == 1) {
                  cli_print("ERR: operacion no permitida o parametros invalidos\r\n");
                  return;
              }
					  cli_print("OK\r\n");
					  return;
					}

					if (strcmp(st, "off") == 0) {
						if (Controlador_BypassBanco(b) == 1) {
                  cli_print("ERR: operacion no permitida o parametros invalidos\r\n");
                  return;
              }
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
	              banco_user > Controlador_GetCantBancos()) {

	              cli_print("ERR: banco fuera de rango\r\n");
	              return;
	          }

	          if (celda_user < 1 ||
	              celda_user > Controlador_GetCantCeldas()) {

	              cli_print("ERR: celda fuera de rango\r\n");
	              return;
	          }

	          if (periodo < PERIODO_DEFAULT || periodo > UINT16_MAX) {
	              cli_print("ERR: periodo minimo 50 ms\r\n");
	              return;
	          }

	          if (modo != 's' && modo != 'c') {
	              cli_print("ERR: modo invalido (use s o c)\r\n");
	              return;
	          }

	          banco = (uint8_t)(banco_user - 1);
	          celda = (uint8_t)(celda_user - 1);

	          if (Controlador_ModificarPeriodo(
	              (uint16_t)periodo
	          ) == 1) {
                  cli_print("ERR: operacion no permitida o parametros invalidos\r\n");
                  return;
              }

	          if (Controlador_IniciarSwitchingCelda(
	              banco,
	              celda
	          ) == 1) {
                  cli_print("ERR: operacion no permitida o parametros invalidos\r\n");
                  return;
              }

	          if (Controlador_ModificarModoCelda(
	              banco,
	              celda,
	              modo
	          ) == 1) {
                  cli_print("ERR: operacion no permitida o parametros invalidos\r\n");
                  return;
              }

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
	              banco_user > Controlador_GetCantBancos()) {

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
	              valor <= Controlador_GetCantCeldas()) {

	              celda = (uint8_t)(valor - 1);

	              if (Controlador_IniciarSwitchingCelda(
	                  banco,
	                  celda
	              ) == 1) {
                  cli_print("ERR: operacion no permitida o parametros invalidos\r\n");
                  return;
              }

	              if (Controlador_ModificarModoCelda(
	                  banco,
	                  celda,
	                  modo
	              ) == 1) {
                  cli_print("ERR: operacion no permitida o parametros invalidos\r\n");
                  return;
              }

	              cli_print("OK\r\n");
	              return;
	          }

	          /* CASO 2B: sw <banco> <periodo> <modo> */
	          if (valor >= PERIODO_DEFAULT && valor <= UINT16_MAX) {

	              if (Controlador_ModificarPeriodo(
	                  (uint16_t)valor
	              ) == 1) {
                  cli_print("ERR: operacion no permitida o parametros invalidos\r\n");
                  return;
              }

	              if (Controlador_IniciarSwitchingBanco(
	                  banco
	              ) == 1) {
                  cli_print("ERR: operacion no permitida o parametros invalidos\r\n");
                  return;
              }

	              if (Controlador_ModificarModo(
	                  banco,
	                  modo
	              ) == 1) {
                  cli_print("ERR: operacion no permitida o parametros invalidos\r\n");
                  return;
              }

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
	              banco_user > Controlador_GetCantBancos()) {

	              cli_print("ERR: banco fuera de rango\r\n");
	              return;
	          }

	          if (celda_user < 1 ||
	              celda_user > Controlador_GetCantCeldas()) {

	              cli_print("ERR: celda fuera de rango\r\n");
	              return;
	          }

	          banco = (uint8_t)(banco_user - 1);
	          celda = (uint8_t)(celda_user - 1);

	          if (Controlador_PararSwitchingCelda(
	              banco,
	              celda
	          ) == 1) {
                  cli_print("ERR: operacion no permitida o parametros invalidos\r\n");
                  return;
              }

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
	              banco_user > Controlador_GetCantBancos()) {

	              cli_print("ERR: banco fuera de rango\r\n");
	              return;
	          }

	          if (modo != 's' && modo != 'c') {
	              cli_print("ERR: modo invalido (use s o c)\r\n");
	              return;
	          }

	          banco = (uint8_t)(banco_user - 1);

	          if (Controlador_IniciarSwitchingBanco(
	              banco
	          ) == 1) {
                  cli_print("ERR: operacion no permitida o parametros invalidos\r\n");
                  return;
              }

	          if (Controlador_ModificarModo(
	              banco,
	              modo
	          ) == 1) {
                  cli_print("ERR: operacion no permitida o parametros invalidos\r\n");
                  return;
              }

	          cli_print("OK\r\n");
	          return;
	      }


	      /* CASO 5: sw <banco> off */
	      if (sscanf(line_copy,
	                 "sw %u off %c",
	                 &banco_user,
	                 &extra) == 1) {

	          if (banco_user < 1 ||
	              banco_user > Controlador_GetCantBancos()) {

	              cli_print("ERR: banco fuera de rango\r\n");
	              return;
	          }

	          banco = (uint8_t)(banco_user - 1);

	          if (Controlador_DetenerSwitchingBancoBypass(
	              banco
	          ) == 1) {
                  cli_print("ERR: operacion no permitida o parametros invalidos\r\n");
                  return;
              }

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
	//HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
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














