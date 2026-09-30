#include "diagnostico.h"
#include "main.h"
static diagnostico_evento_t historial[DIAGNOSTICO_CAPACIDAD];
static uint8_t siguiente, cantidad;

resultado_t Diagnostico_Registrar(resultado_t codigo, const char *funcion, uint32_t detalle)
{
    if (codigo == RES_OK) return codigo;
    historial[siguiente] = (diagnostico_evento_t){codigo, funcion, HAL_GetTick(), detalle};
    siguiente = (siguiente + 1u) % DIAGNOSTICO_CAPACIDAD;
    if (cantidad < DIAGNOSTICO_CAPACIDAD) cantidad++;
    return codigo;
}
uint8_t Diagnostico_EsError(resultado_t codigo)
{
    return codigo > RES_OK && codigo < ADV_BARRIDO_CANCELADO;
}
uint8_t Diagnostico_Cantidad(void) { return cantidad; }
uint8_t Diagnostico_Get(uint8_t indice, diagnostico_evento_t *evento)
{
    if (evento == NULL || indice >= cantidad) return 0u;
    uint8_t inicio = (siguiente + DIAGNOSTICO_CAPACIDAD - cantidad) % DIAGNOSTICO_CAPACIDAD;
    *evento = historial[(inicio + indice) % DIAGNOSTICO_CAPACIDAD];
    return 1u;
}
void Diagnostico_Limpiar(void) { siguiente = cantidad = 0u; }
const char *Diagnostico_Texto(resultado_t codigo)
{
    switch (codigo) {
    case RES_OK: return "OK";
    case ERR_CONFIGURACION: return "cantidad de bancos/celdas invalida";
    case ERR_NO_INICIALIZADO: return "ejecute init primero";
    case ERR_BANCO: return "banco fuera de la matriz configurada";
    case ERR_CELDA: return "celda fuera de la matriz configurada";
    case ERR_ESTADO: return "operacion no permitida en el estado actual";
    case ERR_MODO: return "modo invalido (s/c/f)";
    case ERR_PERIODO: return "periodo fuera de 50..65535 ms";
    case ERR_OCUPADO: return "barrido ya en curso";
    case ERR_ADC_CALIBRACION: return "fallo de calibracion ADC";
    case ERR_ADC_INICIO: return "fallo al iniciar ADC/DMA";
    case ERR_ADC_INACTIVO: return "ADC/DMA no iniciado";
    case ERR_SIN_DATOS: return "medicion aun no disponible";
    case ERR_PUNTERO: return "puntero nulo";
    case ERR_BARRIDO_TIMEOUT: return "barrido excedio el tiempo limite";
    case ERR_COMANDO: return "comando o argumentos invalidos; use help";
    case ERR_LINEA_LARGA: return "linea demasiado larga; comando descartado";
    case ERR_BUFFER: return "buffer de mediciones demasiado corto";
    case ADV_BARRIDO_CANCELADO: return "barrido cancelado; resultados parciales";
    case ADV_LIMITE_ADC: return "lectura en limite ADC; no confirma falla de celda";
    case ADV_DATOS_ANTIGUOS: return "medicion con mas de 1000 ms";
    case ADV_LINEA_PERDIDA: return "comando descartado: otro estaba pendiente";
    default: return "codigo desconocido";
    }
}
