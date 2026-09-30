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
static uint8_t line_overflow;
static volatile uint8_t rx_line_error;
static volatile uint8_t rx_line_lost;

static volatile uint8_t cli_line_ready = 0;
static char cli_cmd_buf[CLI_BUF_LEN];


static void cli_print(const char *s)
{
  HAL_UART_Transmit(cli_huart, (uint8_t *)s, (uint16_t)strlen(s), HAL_MAX_DELAY);
}

static void cli_resultado(resultado_t resultado)
{
    char mensaje[112];
    snprintf(mensaje, sizeof(mensaje), "%s: %s\r\n",
             resultado == RES_OK ? "OK" : Diagnostico_EsError(resultado) ? "ERR" : "ADV",
             Diagnostico_Texto(resultado));
    cli_print(mensaje);
}

static void cli_error(resultado_t codigo, uint32_t detalle)
{
    cli_resultado(Diagnostico_Registrar(codigo, "CLI", detalle));
}

static void cli_diagnostico(void)
{
    char mensaje[160];
    uint8_t n = Diagnostico_Cantidad();
    if (n == 0) { cli_print("Sin errores ni advertencias registrados.\r\n"); return; }
    cli_print("Historial (ultimos 16; no indica fallas electricas activas):\r\n");
    for (uint8_t i = 0; i < n; i++) {
        diagnostico_evento_t evento;
        if (!Diagnostico_Get(i, &evento)) continue;
        snprintf(mensaje, sizeof(mensaje), "%lu ms %s %s: %s (detalle=0x%lX)\r\n",
                 (unsigned long)evento.instante_ms,
                 Diagnostico_EsError(evento.codigo) ? "ERR" : "ADV",
                 evento.funcion, Diagnostico_Texto(evento.codigo),
                 (unsigned long)evento.detalle);
        cli_print(mensaje);
    }
}

static void cli_advertir_antiguedad(uint32_t instante_ms)
{
    uint32_t edad = HAL_GetTick() - instante_ms;
    if (edad > 1000u) {
        cli_resultado(Diagnostico_Registrar(ADV_DATOS_ANTIGUOS, __func__, edad));
    }
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


static void escribir_estado_controlador(void)
{
    static const char *const nombres[] = {
        [INIT]     = "INIT",
        [DESCARGA] = "DESCARGA",
        [CARGA]    = "CARGA",
        [REPOSO]   = "REPOSO",
        [ABIERTO]  = "ABIERTO",
        [FALLA]    = "FALLA",
		[PRUEBA]   = "PRUEBA"
    };

    controlador_estado_t estado = Controlador_GetEstado();

    const char *nombre = (unsigned)estado <
        (sizeof(nombres) / sizeof(nombres[0]))
        ? nombres[estado]
        : "DESCONOCIDO";

    char mensaje[48];
    snprintf(mensaje, sizeof(mensaje),
             "Estado controlador: %s\r\n", nombre);
    cli_print(mensaje);
}


static void cli_print_status(void)
{
  char buf[160];
  uint8_t celdas[CELDAS_POR_BANCO] = {OFF};

  escribir_estado_controlador();


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
                      (c < Controlador_GetCantCeldas() - 1) ? " | " : "");
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
        cli_error(ERR_SIN_DATOS, 0u);
        return;
    }
    cli_advertir_antiguedad(adc_get_processed_ms());
    snprintf(mensaje, sizeof(mensaje), "Tensión en la salida: %u mV\r\n",
                 (unsigned)Controlador_GetTensionSalida_mV());
    cli_print(mensaje);
    for (uint8_t b = 0; b < Controlador_GetCantBancos(); b++) {
        snprintf(mensaje, sizeof(mensaje),
                 "Banco %u: %u mV \r\n",
                 (unsigned)(b + 1u),
                 (unsigned)Controlador_GetTensionBanco_mV(b));
        cli_print(mensaje);
    }

    snprintf(mensaje, sizeof(mensaje), "Corriente de descarga: %u mA\r\n",
             (unsigned)Controlador_GetCorrienteDescarga_mA());
    cli_print(mensaje);
    snprintf(mensaje, sizeof(mensaje), "Corriente de carga: %u mA\r\n",
             (unsigned)Controlador_GetCorrienteCarga_mA());
    cli_print(mensaje);
}

static void cli_print_barrido(void)
{
    static const char *const estados[] = {
        "sin datos", "en curso", "completo", "cancelado", "timeout"
    };
    char mensaje[160];
    snprintf(mensaje, sizeof(mensaje), "Barrido: %s\r\n",
             estados[Controlador_GetEstadoBarrido()]);
    cli_print(mensaje);
    uint8_t advertencia_edad = 0u;
    for (uint8_t b = 0; b < Controlador_GetCantBancos(); b++) {
        for (uint8_t c = 0; c < Controlador_GetCantCeldas(); c++) {
            medicion_celda_t m;
            if (!Controlador_GetMedicionCelda(b, c, &m)) {
                snprintf(mensaje, sizeof(mensaje), "B%u C%u: pendiente\r\n",
                         (unsigned)(b + 1u), (unsigned)(c + 1u));
            } else {
                if (!advertencia_edad && (uint32_t)(HAL_GetTick() - m.instante_ms) > 1000u) {
                    cli_advertir_antiguedad(m.instante_ms);
                    advertencia_edad = 1u;
                }
                snprintf(mensaje, sizeof(mensaje),
                         "B%u C%u: CellNeg=%u mV Banco=%u mV SW=%s Bypass=%s Antiguedad=%lu ms%s\r\n",
                         (unsigned)(b + 1u), (unsigned)(c + 1u),
                         (unsigned)m.cell_neg_mV, (unsigned)m.banco_mV,
                         cli_on_off(m.celda_on), cli_on_off(m.bypass_on),
                         (unsigned long)(HAL_GetTick() - m.instante_ms),
                         m.limite_adc ? " [flag ADC]" : "");
            }
            cli_print(mensaje);
        }
    }

}

static void cli_print_help(void)
{
    cli_print(
        "\r\n"
        "========== AYUDA ==========\r\n"
        "Inicio y estados:\r\n"
        "  init <bancos> <celdas>\r\n"
        "      Bancos: 1..4; celdas por banco: 1..7.\r\n"
        "      Reinicia la configuracion y deshabilita Vcc (INIT).\r\n"
        "      Ejemplo: init 2 3\r\n"
        "  reposo\r\n"
        "      Desde INIT o ABIERTO: habilita Vcc, apaga celdas\r\n"
        "      y activa solo los bypass configurados.\r\n"
		"  prueba\r\n"
		"      Desde INIT o ABIERTO: prueba de LEDs y switches.\r\n"
		"      Sin fallas ni barrido. Usar sin celdas conectadas.\r\n"
		"      Para salir, ejecutar init <bancos> <celdas>.\r\n"
        "  abierto | reset\r\n"
        "      Detiene switching y apaga celdas y bypass (ABIERTO).\r\n"
        "      Mantiene Vcc; no reinicia el microcontrolador.\r\n"
        "  status\r\n"
        "  rl <ohm> | rl\r\n"
        "      Asigna RL (1..65535 ohm enteros) o consulta su valor.\r\n"
        "      Ejemplo: rl 10. init borra RL; sin RL no se verifica I=V/R.\r\n"
        "      En DESCARGA: tolerancia provisional 10% + 50 mA.\r\n"
        "      Muestra los switches de bancos y celdas configurados.\r\n"
        "  help\r\n"
        "      Muestra esta ayuda.\r\n"
        "\r\n"
        "Control manual (requiere haber entrado en reposo):\r\n"
        "  c <banco> <celda> on|off\r\n"
        "      Activa o apaga una celda. Ejemplo: c 1 3 on\r\n"
        "  b <banco> on|off\r\n"
        "      on: activa todas sus celdas; off: deja el bypass ON.\r\n"
        "      Ejemplo: b 1 off\r\n"
        "  Los indices empiezan en 1 y respetan la matriz elegida.\r\n"
        "  El accionamiento se rechaza en INIT y FALLA.\r\n"
        "\r\n"
        "Switching:\r\n"
        "  sw <banco> s|c\r\n"
        "      Todo el banco, con el periodo actual.\r\n"
        "  sw <banco> <periodo_ms> s|c\r\n"
        "      Todo el banco, cambiando el periodo global.\r\n"
        "  sw <banco> <celda> s|c\r\n"
        "      Una celda, con el periodo actual.\r\n"
        "  sw <banco> <celda> <periodo_ms> s|c\r\n"
        "      Una celda, cambiando el periodo global.\r\n"
        "  sw <banco> off\r\n"
        "      Detiene switching del banco; conserva sus estados.\r\n"
        "  sw <banco> <celda> off\r\n"
        "      Detiene esa celda; conserva su estado actual.\r\n"
        "  Modos: s = sincrono; c = complementario.\r\n"
        "  Fase 0: s ON / c OFF. Fase 1: s OFF / c ON.\r\n"
        "  Periodo: 50..65535 ms por fase; ciclo = 2 periodos.\r\n"
        "  Sin periodo previo se usan 50 ms por fase.\r\n"
        "  En sw <banco> <numero> s|c, numero < 50 es celda;\r\n"
        "  numero >= 50 es periodo. La celda debe estar configurada.\r\n"
        "  Ejemplo de bancos intercalados: sw 1 500 s / sw 2 c\r\n"
        "\r\n"
        "Mediciones:\r\n"
        "  medir\r\n"
        "      Muestra tension de salida y bancos en mV,\r\n"
        "      y corrientes de carga/descarga en mA.\r\n"
        "  mux <celda>\r\n"
        "      Selecciona esa celda en todos los MUX y cancela\r\n"
        "      cualquier barrido activo. Ejemplo: mux 2\r\n"
        "  barrer\r\n"
        "      Inicia un barrido no bloqueante de la matriz configurada.\r\n"
        "      Requiere matriz habilitada, ADC activo y barrido libre.\r\n"
        "      No cambia los switches; deja el MUX en la ultima celda.\r\n"
        "  barrido\r\n"
        "      Muestra avance/resultados: CellNeg, tension de banco,\r\n"
        "      switches y antiguedad. Pendiente = aun sin muestra.\r\n"
        "      CellNeg es respecto de GND, NO tension de la celda.\r\n"
        "      [limite ADC] indica lectura en un extremo del ADC;\r\n"
        "      no confirma una falla ni permite medir tension negativa.\r\n"
        "\r\n"
        "Diagnostico:\r\n"
        "  diagnostico\r\n"
        "      Muestra los ultimos 16 errores/advertencias registrados,\r\n"
        "      con funcion, instante en ms desde arranque y detalle.\r\n"
        "  diagnostico limpiar\r\n"
        "      Borra solo el historial; no rearma fallas electricas.\r\n"
        "  OK: operacion aceptada. ERR: operacion rechazada.\r\n"
        "  ADV: advertencia informativa; no corta la llave.\r\n"
        "  Datos de mas de 1000 ms generan advertencia al consultarlos.\r\n"
        "  Lineas de mas de 63 caracteres se descartan completas.\r\n"
        "  Antes de init estan disponibles help y diagnostico.\r\n"
        "\r\n"
        "Ejemplo (un comando por linea):\r\n"
        "  init 2 3\r\n"
        "  reposo\r\n"
        "  c 1 1 on\r\n"
        "  medir\r\n"
        "  barrer\r\n"
        "  barrido\r\n"
        "===========================\r\n"
    );
}


/* ===================== MANEJO DE COMANDOS===================== */

/* Conversion estricta antes de estrechar a uint8_t/uint16_t. */
static uint8_t cli_numero(const char *texto, uint32_t *valor)
{
    uint32_t n = 0;
    if (*texto == '\0') return 0u;
    for (; *texto; texto++) {
        if (*texto < '0' || *texto > '9') return 0u;
        uint32_t digito = (uint32_t)(*texto - '0');
        if (n > (UINT32_MAX - digito) / 10u) return 0u;
        n = n * 10u + digito;
    }
    *valor = n;
    return 1u;
}

static void cli_handle_line(const char *line_in)
{
    char copia[CLI_BUF_LEN];
    char *args[6];
    unsigned n = 0;
    strncpy(copia, line_in, sizeof(copia));
    copia[sizeof(copia) - 1] = '\0';
    str_to_lower(copia);
    for (char *p = strtok(copia, " \t"); p; p = strtok(NULL, " \t")) {
        if (n == 6) { cli_error(ERR_COMANDO, n); return; }
        args[n++] = p;
    }
    if (!n) return;
    if (strcmp(args[0], "help") == 0 && n == 1) { cli_print_help(); return; }
    if (strcmp(args[0], "diagnostico") == 0) {
        if (n == 1) cli_diagnostico();
        else if (n == 2 && strcmp(args[1], "limpiar") == 0) {
            Diagnostico_Limpiar(); cli_print("Historial borrado.\r\n");
        } else cli_error(ERR_COMANDO, n);
        return;
    }
    uint32_t banco, celda, valor, periodo = 0;
    resultado_t r;
    if (strcmp(args[0], "init") == 0) {
        if (n != 3 || !cli_numero(args[1], &banco) || !cli_numero(args[2], &celda)) {
            cli_error(ERR_COMANDO, n); return;
        }
        if (banco < 1 || banco > CANT_BANCOS || celda < 1 || celda > CELDAS_POR_BANCO) {
            cli_error(ERR_CONFIGURACION, 0u); return;
        }
        r = Controlador_init((uint8_t)banco, (uint8_t)celda);
        cli_resultado(r);
        if (r == RES_OK) cli_print("Vcc deshabilitado; use reposo o prueba para habilitar.\r\n");
        return;
    }
    if (Controlador_GetCantBancos() == 0) { cli_error(ERR_NO_INICIALIZADO, 0u); return; }
    if (strcmp(args[0], "rl") == 0) {
        if (n == 2) {
            if (!cli_numero(args[1], &valor) || valor == 0 || valor > UINT16_MAX) {
                cli_error(ERR_CONFIGURACION, 0u); return;
            }
            r = Controlador_SetResistenciaCarga((uint16_t)valor);
            if (r != RES_OK) { cli_resultado(r); return; }
        } else if (n != 1) { cli_error(ERR_COMANDO, n); return; }
        char mensaje[96];
        uint16_t rl = Controlador_GetResistenciaCarga();
        snprintf(mensaje, sizeof(mensaje), "RL=%u ohm%s\r\n", (unsigned)rl,
                 rl ? "" : " (sin configurar; comprobacion RL deshabilitada)");
        cli_print(mensaje);
        return;
    }
    if (n == 1) {
    	if (strcmp(args[0], "prueba") == 0) {
    	    r = Controlador_EntrarPrueba();
    	    cli_resultado(r);

    	    if (r == RES_OK) {
    	        cli_print("Modo PRUEBA: sin evaluacion de fallas ni barrido.\r\n");
    	        escribir_estado_controlador();
    	        cli_print_status();
    	    }
    	}
    	else if (strcmp(args[0], "reposo") == 0) {
            r = Controlador_EntrarReposo(); cli_resultado(r);
            if (r == RES_OK) cli_print_status();
        } else if (strcmp(args[0], "status") == 0) cli_print_status();
        else if (strcmp(args[0], "reset") == 0 || strcmp(args[0], "abierto") == 0)
            cli_resultado(Controlador_EstadoAbierto());
        else if (strcmp(args[0], "medir") == 0) {
            adc_update(); Controlador_CargarMediciones(); cli_print_mediciones();
        } else if (strcmp(args[0], "barrido") == 0) cli_print_barrido();
        else if (strcmp(args[0], "barrer") == 0) {
            if (Controlador_IniciarBarrido()) cli_print("Barrido iniciado. Consulte barrido.\r\n");
            else {
                diagnostico_evento_t evento;
                if (Diagnostico_Get(Diagnostico_Cantidad() - 1u, &evento)) cli_resultado(evento.codigo);
            }
        } else cli_error(ERR_COMANDO, n);
        return;
    }
    if (strcmp(args[0], "mux") == 0) {
        if (n != 2 || !cli_numero(args[1], &celda)) { cli_error(ERR_COMANDO, n); return; }
        if (celda < 1 || celda > Controlador_GetCantCeldas()) { cli_error(ERR_CELDA, celda); return; }
        cli_resultado(Controlador_SeleccionarMux((uint8_t)(celda - 1u)));
        return;
    }
    if (strcmp(args[0], "c") != 0 && strcmp(args[0], "b") != 0 && strcmp(args[0], "sw") != 0) {
        cli_error(ERR_COMANDO, n); return;
    }
    if (!cli_numero(args[1], &banco)) { cli_error(ERR_COMANDO, n); return; }
    if (banco < 1 || banco > Controlador_GetCantBancos()) { cli_error(ERR_BANCO, banco); return; }
    uint8_t b = (uint8_t)(banco - 1u);
    if (strcmp(args[0], "c") == 0) {
        if (n != 4 || !cli_numero(args[2], &celda)) { cli_error(ERR_COMANDO, n); return; }
        if (celda < 1 || celda > Controlador_GetCantCeldas()) { cli_error(ERR_CELDA, celda); return; }
        if (strcmp(args[3], "on") == 0) r = Controlador_EncenderCelda(b, (uint8_t)(celda - 1u));
        else if (strcmp(args[3], "off") == 0) r = Controlador_ApagarCelda(b, (uint8_t)(celda - 1u));
        else { cli_error(ERR_COMANDO, n); return; }
        cli_resultado(r); return;
    }
    if (strcmp(args[0], "b") == 0) {
        if (n != 3) { cli_error(ERR_COMANDO, n); return; }
        if (strcmp(args[2], "on") == 0) r = Controlador_ActivarCeldasBanco(b);
        else if (strcmp(args[2], "off") == 0) r = Controlador_BypassBanco(b);
        else { cli_error(ERR_COMANDO, n); return; }
        cli_resultado(r); return;
    }
    if (n < 3 || n > 5) { cli_error(ERR_COMANDO, n); return; }
    const char *modo = args[n - 1];
    uint8_t detener = strcmp(modo, "off") == 0;
    if (!detener && strcmp(modo, "s") != 0 && strcmp(modo, "c") != 0) {
        cli_error(ERR_MODO, 0u); return;
    }
    uint8_t individual = 0;
    celda = 0;
    if (n >= 4) {
        if (!cli_numero(args[2], &valor)) { cli_error(ERR_COMANDO, n); return; }
        if (n == 5 || detener || valor < PERIODO_DEFAULT) {
            if (valor < 1 || valor > Controlador_GetCantCeldas()) { cli_error(ERR_CELDA, valor); return; }
            individual = 1; celda = valor - 1u;
        } else periodo = valor;
    }
    if (n == 5) {
        if (detener || !cli_numero(args[3], &periodo)) { cli_error(ERR_COMANDO, n); return; }
        if (periodo < PERIODO_DEFAULT) { cli_error(ERR_PERIODO, periodo); return; }
    }
    if (periodo > UINT16_MAX) { cli_error(ERR_PERIODO, periodo); return; }
    if (detener) {
        r = individual ? Controlador_PararSwitchingCelda(b, (uint8_t)celda)
                       : Controlador_DetenerSwitchingBancoBypass(b);
    } else {
        if (periodo) {
            r = Controlador_ModificarPeriodo((uint16_t)periodo);
            if (r != RES_OK) { cli_resultado(r); return; }
        }
        r = individual ? Controlador_IniciarSwitchingCelda(b, (uint8_t)celda)
                       : Controlador_IniciarSwitchingBanco(b);
        if (r == RES_OK) r = individual ? Controlador_ModificarModoCelda(b, (uint8_t)celda, modo[0])
                                        : Controlador_ModificarModo(b, modo[0]);
    }
    cli_resultado(r);
}

/* ===================== API ===================== */

void CLI_Init(UART_HandleTypeDef *huart)
{
  cli_huart = huart;
  line_len = 0;

  cli_print("\r\nConfigurar matriz: init <bancos 1..4> <celdas por banco 1..7>\r\n> ");
  HAL_UART_Receive_IT(cli_huart, &rx_ch, 1);
}

void CLI_RxCallback(UART_HandleTypeDef *huart)
{
    if (huart != cli_huart) return;
    if (rx_ch == '\r' || rx_ch == '\n') {
        if (line_overflow) rx_line_error = 1u;
        else if (line_len > 0) {
            if (cli_line_ready) rx_line_lost = 1u;
            else {
                line_buf[line_len] = '\0';
                strcpy(cli_cmd_buf, line_buf);
                cli_line_ready = 1u;
            }
        }
        line_len = line_overflow = 0u;
    } else if (line_len < CLI_BUF_LEN - 1) {
        line_buf[line_len++] = (char)rx_ch;
    } else line_overflow = 1u;
    HAL_UART_Receive_IT(cli_huart, &rx_ch, 1);
}



void CLI_Process(void)
{
    char comando[CLI_BUF_LEN];
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    uint8_t listo = cli_line_ready;
    uint8_t larga = rx_line_error, perdida = rx_line_lost;
    if (listo) { strcpy(comando, cli_cmd_buf); cli_line_ready = 0u; }
    rx_line_error = rx_line_lost = 0u;
    __set_PRIMASK(primask);

    if (larga) cli_error(ERR_LINEA_LARGA, CLI_BUF_LEN);
    if (perdida) cli_resultado(Diagnostico_Registrar(ADV_LINEA_PERDIDA, __func__, 0u));
    if (!listo) return;
    cli_handle_line(comando);
    cli_print("> ");
}
