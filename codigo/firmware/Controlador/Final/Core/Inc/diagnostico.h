#ifndef INC_DIAGNOSTICO_H_
#define INC_DIAGNOSTICO_H_
#include <stdint.h>

#define DIAGNOSTICO_CAPACIDAD 16u
/* OK=0. Errores rechazan la operacion; advertencias son informativas.
 * Ninguno de estos codigos es una falla electrica ni acciona la llave. */
typedef enum {
    RES_OK = 0,
    ERR_CONFIGURACION, ERR_NO_INICIALIZADO, ERR_BANCO, ERR_CELDA,
    ERR_ESTADO, ERR_MODO, ERR_PERIODO, ERR_OCUPADO,
    ERR_ADC_CALIBRACION, ERR_ADC_INICIO, ERR_ADC_INACTIVO,
    ERR_SIN_DATOS, ERR_PUNTERO, ERR_BARRIDO_TIMEOUT, ERR_COMANDO,
    ERR_LINEA_LARGA, ERR_BUFFER,
    ADV_BARRIDO_CANCELADO = 100,
    ADV_LIMITE_ADC, ADV_DATOS_ANTIGUOS, ADV_LINEA_PERDIDA
} resultado_t;

typedef struct {
    resultado_t codigo;
    const char *funcion; /* Debe ser literal o __func__, nunca buffer temporal. */
    uint32_t instante_ms;
    uint32_t detalle;
} diagnostico_evento_t;

/* API desde main, no desde ISR. Historial circular, no estados activos. */
resultado_t Diagnostico_Registrar(resultado_t codigo, const char *funcion, uint32_t detalle);
uint8_t Diagnostico_EsError(resultado_t codigo);
const char *Diagnostico_Texto(resultado_t codigo);
uint8_t Diagnostico_Cantidad(void);
uint8_t Diagnostico_Get(uint8_t indice, diagnostico_evento_t *evento);
void Diagnostico_Limpiar(void);
#endif
