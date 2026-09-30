/*
 * controlador.c
 *
 *  Created on: 10 jun 2026
 *      Author: ---
 */

#include "controlador.h"
#include "main.h"
#include "llave.h"
#include "fallas.h"
#include <string.h>

#define TIEMPO_ESTABILIZACION_SR_MS 0u
#define UMBRAL_CORRIENTE_MA 50  //todo

static controlador_t ctrl;

/* Valores iniciales para ensayos: validar asentamiento del MUX en placa. */
#define MUX_ASENTAMIENTO_MS 2u
#define BARRIDO_TIMEOUT_MS 2000u

static medicion_celda_t mediciones_celdas[CANT_BANCOS][CELDAS_POR_BANCO];
static barrido_estado_t estado_barrido = BARRIDO_SIN_DATOS;
static uint8_t barrido_celda;
static uint8_t barrido_espera_adc;
static uint32_t barrido_inicio_ms, barrido_seleccion_ms;
static uint32_t barrido_adc_marca;
static uint32_t salidas_version, barrido_salidas_version;
static uint32_t salidas_adc_marca;
static uint32_t encendido_adc_marca;
static uint8_t vcc_habilitada;
static uint8_t chequeo_inicial_pendiente;


extern volatile uint8_t flag_controlador_update;


static shift_register_t sr_bancos[CANT_BANCOS];
static mux_t mux_bancos[CANT_BANCOS]; //TODO pasar el manejo de mux a un unico mux



static uint16_t periodo_ms = 0;
static uint16_t contador = 0;
static uint8_t fase = 0;


//por ahora solo tengo el banco 1
static GPIO_TypeDef *sr_data_puertos[CANT_BANCOS] = {
	DATA1_GPIO_Port,
	DATA2_GPIO_Port,
	DATA3_GPIO_Port,
	DATA4_GPIO_Port,
};

static uint16_t sr_data_pines[CANT_BANCOS] = {
	DATA1_Pin,
	DATA2_Pin,
	DATA3_Pin,
	DATA4_Pin,
};



int verificar_banco(uint8_t banco);

#define RECHAZAR(codigo, detalle) return Diagnostico_Registrar(codigo, __func__, detalle)

static resultado_t Controlador_ValidarOperacion(void)
{
    if (ctrl.cant_bancos == 0) return ERR_NO_INICIALIZADO;
    if (ctrl.estado == INIT || ctrl.estado == FALLA) return ERR_ESTADO;
    if (chequeo_inicial_pendiente) return ERR_OCUPADO;
    return RES_OK;
}
static void SeleccionarMux(uint8_t celda);

/* Llamar con los SR alimentados. Forzar tambien los bits no configurados. */
static void Controlador_EnviarTodoApagado(void)
{
    periodo_ms = contador = fase = 0;
    for (uint8_t b = 0; b < CANT_BANCOS; b++) {
        ctrl.bancos[b].prox = OFF;
        for (uint8_t c = 0; c < CELDAS_POR_BANCO; c++) {
            ctrl.bancos[b].celdas[c].prox = OFF;
            ctrl.bancos[b].celdas[c].modo = FIJO;
        }
    }
    /* No usar deteccion de cambios: siempre transmitir el frame completo. */
    for (int8_t pin = CANT_PINES_SR - 1; pin >= 0; pin--) {
        for (uint8_t b = 0; b < CANT_BANCOS; b++)
            Banco_AplicarEstadoPin(&ctrl.bancos[b], pin);
        Banco_ClockPulse(&ctrl.bancos[0]);
    }
    Banco_LatchPulse(&ctrl.bancos[0]);
    salidas_version++;
    salidas_adc_marca = adc_get_conversion_sequence();
}


resultado_t Controlador_init(uint8_t bancos, uint8_t celdas)
{
    if (bancos == 0 || bancos > CANT_BANCOS ||
        celdas == 0 || celdas > CELDAS_POR_BANCO) RECHAZAR(ERR_CONFIGURACION, ((uint32_t)bancos << 8) | celdas);

    Controlador_CancelarBarrido();
    if (vcc_habilitada) Controlador_EnviarTodoApagado();
    memset(mediciones_celdas, 0, sizeof(mediciones_celdas));
    estado_barrido = BARRIDO_SIN_DATOS;
    Llave_Init();
    vcc_habilitada = 0;
    chequeo_inicial_pendiente = 0;
    ctrl = (controlador_t){0};
    periodo_ms = 0;
    contador = 0;
    fase = 0;
    ctrl.cant_bancos = bancos;

    /* Preparar todos los SR antes de pulsar el clock/latch compartido.
     * Los bancos no configurados y las celdas sobrantes quedan apagados. */
    for (uint8_t i = 0; i < CANT_BANCOS; i++) {
        ctrl.bancos[i].id = i;

        SR_Init(&sr_bancos[i],
                sr_data_puertos[i], sr_data_pines[i],
                CLK_GPIO_Port, CLK_Pin,
                LATCH_GPIO_Port, LATCH_Pin);

        MUX_Init(&mux_bancos[i],
                 S2_GPIO_Port, S2_Pin,
                 S1_GPIO_Port, S1_Pin,
                 S0_GPIO_Port, S0_Pin);

        MUX_SetNodo(&mux_bancos[i], ADC_MUX_BANCO_1 + i);

        Banco_Init(&ctrl.bancos[i],
                   &sr_bancos[i],
                   &mux_bancos[i],
                   i < bancos ? celdas : 0,
                   ADC_MUX_BANCO_1 + i);
    }
    ctrl.resistencia_carga = 0;

    return RES_OK;
}


resultado_t Controlador_EntrarReposo(void)
{
    if ((ctrl.estado != INIT && ctrl.estado != ABIERTO) || ctrl.cant_bancos == 0) {
        RECHAZAR(ctrl.cant_bancos == 0 ? ERR_NO_INICIALIZADO : ERR_ESTADO, ctrl.estado);
    }

    periodo_ms = 0;
    contador = 0;
    fase = 0;

    /* Preparar el apagado de todos los bancos. */
    for (uint8_t i = 0; i < CANT_BANCOS; i++) {
        Banco_SetModo(&ctrl.bancos[i], FIJO);
        Banco_ApagarCeldas(&ctrl.bancos[i]);
        Banco_ApagarSwitch(&ctrl.bancos[i]);

        SR_SetData(&sr_bancos[i], OFF);
    }

    HAL_GPIO_WritePin(CLK_GPIO_Port, CLK_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LATCH_GPIO_Port, LATCH_Pin, GPIO_PIN_RESET);


    Llave_Habilitar();
    vcc_habilitada = 1;
    HAL_Delay(TIEMPO_ESTABILIZACION_SR_MS);

    /*
     * Forzar el primer frame completo en OFF.
     */
    for (int8_t pin = CANT_PINES_SR - 1; pin >= 0; pin--) {
        for (uint8_t i = 0; i < CANT_BANCOS; i++) {
            Banco_AplicarEstadoPin(&ctrl.bancos[i], pin);
        }

        Banco_ClockPulse(&ctrl.bancos[0]);
    }

    Banco_LatchPulse(&ctrl.bancos[0]);

    /* Encender solo los bypass configurados. */
    for (uint8_t i = 0; i < ctrl.cant_bancos; i++) {
        Banco_EncenderSwitch(&ctrl.bancos[i]);
    }

    Controlador_AplicarEstados();
    encendido_adc_marca = adc_get_conversion_sequence();
    chequeo_inicial_pendiente = 1;
    ctrl.estado = REPOSO;
    return RES_OK;
}

resultado_t Controlador_EntrarPrueba(void)
{
    resultado_t resultado = Controlador_EntrarReposo();

    if (resultado != RES_OK) return resultado;

    Controlador_CancelarBarrido();
    chequeo_inicial_pendiente = 0;
    ctrl.estado = PRUEBA;

    return RES_OK;
}

uint8_t Controlador_GetCantBancos(void)
{
    return ctrl.cant_bancos;
}

resultado_t Controlador_SetResistenciaCarga(uint16_t resistencia_ohm)
{
    if (ctrl.cant_bancos == 0) RECHAZAR(ERR_NO_INICIALIZADO, 0u);
    if (resistencia_ohm == 0) RECHAZAR(ERR_CONFIGURACION, 0u);
    ctrl.resistencia_carga = resistencia_ohm;
    return RES_OK;
}

uint16_t Controlador_GetResistenciaCarga(void)
{
    return ctrl.resistencia_carga;
}

uint8_t Controlador_GetCantCeldas(void)
{
    return ctrl.cant_bancos ? ctrl.bancos[0].cant_celdas : 0;
}




void Controlador_Update(void)
{
    if (ctrl.estado == PRUEBA) {
        Controlador_Tick1ms();
        Controlador_ActualizarEstados();
        return;
    }

    if (Controlador_EvaluarFallas()) {
        Controlador_CambiarEstado(FALLA);
        return;
    }

    if (chequeo_inicial_pendiente) return;

    Controlador_EvaluarEstado();
    if (ctrl.estado == FALLA) return;

    Controlador_Tick1ms();
    Controlador_ActualizarEstados();
}

int Controlador_EvaluarFallas(void)
{
	if (ctrl.estado == PRUEBA) return 0;
    if (ctrl.estado == FALLA) return 1;
    if (ctrl.cant_bancos == 0 || ctrl.estado == INIT) return 0;
    if (!adc_is_dma_started() || !adc_has_measurements()) return 0;
    if (chequeo_inicial_pendiente &&
        (uint32_t)(adc_get_processed_sequence() - encendido_adc_marca) < 2u)
        return 0;

    if (Fallas_EvaluarCorrientes(
        Controlador_GetCorrienteDescarga_mA(),
        Controlador_GetCorrienteCarga_mA()
    ) != FALLA_NINGUNA) return 1;

    /* Esperar un bloque completo posterior al ultimo cambio de switches. */
    if ((uint32_t)(adc_get_processed_sequence() - salidas_adc_marca) < 2u)
        return 0;

    for (uint8_t b = 0; b < ctrl.cant_bancos; b++) {
        if (ctrl.bancos[b].actual == ON ||
            !Banco_HayCeldasActualON(&ctrl.bancos[b])) continue;

        /* Aproximacion por tension del banco activo, no un diagnostico
         * de una celda individual. */
        if (Fallas_EvaluarTensionCelda(Controlador_GetTensionBanco_mV(b))
            != FALLA_NINGUNA) return 1;
    }
    chequeo_inicial_pendiente = 0;
    if (ctrl.estado == DESCARGA && Fallas_EvaluarDescargaRL(
            Controlador_GetTensionSalida_mV(),
            Controlador_GetCorrienteDescarga_mA(), ctrl.resistencia_carga)
            != FALLA_NINGUNA) return 1;
    return 0;
}

void Controlador_EvaluarEstado(void)
{
	if (ctrl.estado == PRUEBA) return;
    if (ctrl.estado == INIT || ctrl.estado == ABIERTO ||
        ctrl.estado == FALLA) return;

    if (Controlador_EvaluarFallas()) {
        Controlador_CambiarEstado(FALLA);
        return;
    }
    if (chequeo_inicial_pendiente) return;

    if (Controlador_GetCorrienteDescarga_mA() > UMBRAL_CORRIENTE_MA) {
        Controlador_CambiarEstado(DESCARGA);
    }
    else if (Controlador_GetCorrienteCarga_mA() > UMBRAL_CORRIENTE_MA) {
        Controlador_CambiarEstado(CARGA);
    }
}



resultado_t Controlador_CambiarEstado(controlador_estado_t nuevo_estado){
    if ((unsigned)nuevo_estado > FALLA) RECHAZAR(ERR_ESTADO, nuevo_estado);
    if (ctrl.estado == FALLA && nuevo_estado != FALLA)
        RECHAZAR(ERR_ESTADO, nuevo_estado);

    if (nuevo_estado == FALLA) {
        Controlador_CancelarBarrido();
        if (vcc_habilitada) Controlador_EnviarTodoApagado();
        Llave_DispararFalla();
        vcc_habilitada = 0;
        chequeo_inicial_pendiente = 0;
        periodo_ms = contador = fase = 0;
        /* Sin Vcc no enviar un frame: limpiar solamente el estado logico. */
        for (uint8_t b = 0; b < CANT_BANCOS; b++) {
            ctrl.bancos[b].actual = ctrl.bancos[b].prox = OFF;
            for (uint8_t c = 0; c < CELDAS_POR_BANCO; c++) {
                ctrl.bancos[b].celdas[c].actual = OFF;
                ctrl.bancos[b].celdas[c].prox = OFF;
                ctrl.bancos[b].celdas[c].modo = FIJO;
            }
        }
    }

	ctrl.estado = nuevo_estado;
    return RES_OK;
}

controlador_estado_t Controlador_GetEstado(void){
	return ctrl.estado;
}

static void Controlador_ClockPulse(void){
	Banco_ClockPulse(&ctrl.bancos[0]);
}


static void Controlador_LatchPulse(void){
	Banco_LatchPulse(&ctrl.bancos[0]);
}



uint8_t Controlador_GetEstadoBanco(uint8_t banco){
    if (verificar_banco(banco) == -1) return OFF;
	return (uint8_t) Banco_GetEstado(&ctrl.bancos[banco]);
}

uint8_t Controlador_GetEstadoCelda(uint8_t banco, uint8_t celda){
    if (verificar_banco(banco) == -1 || celda >= ctrl.bancos[banco].cant_celdas) return OFF;
	return (uint8_t) Banco_GetEstadoCelda(&ctrl.bancos[banco], celda);
}

void Controlador_AplicarEstados(void){
	//Envio 1 bit a la vez de todos los sr y luego clockeo
	if (!Controlador_HayCambios()) {
	        return;
	    }


	for (int8_t  pin = CANT_PINES_SR-1; pin >=0; pin--){
			for (uint8_t sr = 0; sr< CANT_BANCOS; sr++){
				Banco_AplicarEstadoPin(&ctrl.bancos[sr], pin);
			}
			Controlador_ClockPulse();

	}
	Controlador_LatchPulse();
    salidas_version++;
    salidas_adc_marca = adc_get_conversion_sequence();
}




int Controlador_HayCambios(void)
{
    for (uint8_t i = 0; i < CANT_BANCOS; i++) {
        if (Banco_HayCambios(&ctrl.bancos[i])) {
            return 1;
        }
    }

    return 0;
}


// Control individual




resultado_t Controlador_EncenderCelda(uint8_t banco, uint8_t celda)
{
    resultado_t validacion = Controlador_ValidarOperacion();
    if (validacion != RES_OK) RECHAZAR(validacion, ctrl.estado);

    if (verificar_banco(banco) == -1) RECHAZAR(ERR_BANCO, banco);
    if (celda >= ctrl.bancos[banco].cant_celdas) RECHAZAR(ERR_CELDA, celda);


    // Paso 1: solo modifico el banco objetivo.
    // Los otros bancos quedan con su prox intacto.
    if (Banco_GetEstadoActual (&ctrl.bancos[banco]) == ON){
		Banco_ApagarSwitch(&ctrl.bancos[banco]); // sw banco apagado
		Controlador_AplicarEstados(); // escribe TODOS los bancos y hace latch comun
    }

    // Paso 2: prendo la celda.
    Banco_EncenderCelda(&ctrl.bancos[banco], celda);
    Controlador_AplicarEstados(); // otra vez frame completo + latch comun
    if (ctrl.estado == ABIERTO) ctrl.estado = REPOSO;
    return RES_OK;
}



resultado_t Controlador_ApagarCelda(uint8_t banco, uint8_t celda)
{
    resultado_t validacion = Controlador_ValidarOperacion();
    if (validacion != RES_OK) RECHAZAR(validacion, ctrl.estado);

    if (verificar_banco(banco) == -1) RECHAZAR(ERR_BANCO, banco);
    if (celda >= ctrl.bancos[banco].cant_celdas) RECHAZAR(ERR_CELDA, celda);

    //Paso 1: Apago la celda
    Banco_ApagarCelda(&ctrl.bancos[banco],celda);
    Controlador_AplicarEstados(); // escribe TODOS los bancos y hace latch comun



    if (!Banco_HayCeldasProxON(&ctrl.bancos[banco])){ //no quedan celdas en ON->enciendo el banco



    	// Paso 2: prendo la celda.
    	Banco_EncenderSwitch(&ctrl.bancos[banco]);
    	Controlador_AplicarEstados(); // otra vez frame completo + latch comun

    }
    if (ctrl.estado == ABIERTO) ctrl.estado = REPOSO;
    return RES_OK;
}




//Control por bancos

int verificar_banco(uint8_t banco){
	if (banco >= ctrl.cant_bancos )
		return -1;
	return 1;
}

resultado_t Controlador_BypassBanco(uint8_t banco){
    resultado_t validacion = Controlador_ValidarOperacion();
    if (validacion != RES_OK) RECHAZAR(validacion, ctrl.estado);

	if (verificar_banco(banco) == -1) RECHAZAR(ERR_BANCO, banco);

	Banco_ApagarSwitch(&ctrl.bancos[banco]);//solo or si acaso
	Banco_ApagarCeldas(&ctrl.bancos[banco]);
	Controlador_AplicarEstados();



	Banco_EncenderSwitch(&ctrl.bancos[banco]);
    Controlador_AplicarEstados();

    if (ctrl.estado == ABIERTO) ctrl.estado = REPOSO;
    return RES_OK;
}

resultado_t Controlador_ActivarCeldasBanco(uint8_t banco){
    resultado_t validacion = Controlador_ValidarOperacion();
    if (validacion != RES_OK) RECHAZAR(validacion, ctrl.estado);

	if (verificar_banco(banco) == -1) RECHAZAR(ERR_BANCO, banco);

	Banco_ApagarSwitch(&ctrl.bancos[banco]);
	Controlador_AplicarEstados();



	Banco_EncenderCeldas(&ctrl.bancos[banco]);
	Controlador_AplicarEstados();
    if (ctrl.estado == ABIERTO) ctrl.estado = REPOSO;
    return RES_OK;
}



/*
 * Sirve  para actualizar estados con y sin switching
 */
void Controlador_ActualizarEstados(void)
{
	if (ctrl.estado == INIT || ctrl.estado == FALLA || ctrl.estado == ABIERTO) return;
    uint8_t patron[CANT_BANCOS];
    uint8_t hayActual[CANT_BANCOS];
    uint8_t hayProx[CANT_BANCOS];
    uint8_t necesitaPaso2 = 0;

    //if (Controlador_HayCambios() == 0 )	return;
    // 1. Calculo todo primero
    for (uint8_t b = 0; b < ctrl.cant_bancos; b++) {

        patron[b] = Banco_CalcularPatron(&ctrl.bancos[b],fase);
        hayActual[b] = Banco_HayCeldasActualON(&ctrl.bancos[b]);
        hayProx[b] = (patron[b] != 0);
    }

    // 2. Preparo paso 1 para todos los bancos
    for (uint8_t b = 0; b < ctrl.cant_bancos; b++) {
        if (hayActual[b] && hayProx[b]) {
            // ON -> ON
            Banco_ApagarSwitch(&ctrl.bancos[b]);
            Banco_SetPatronCeldas(&ctrl.bancos[b], patron[b]);
        }
        else if (hayActual[b] && !hayProx[b]) {
            // ON -> OFF
            Banco_ApagarSwitch(&ctrl.bancos[b]);
            Banco_ApagarCeldas(&ctrl.bancos[b]);
            necesitaPaso2 = 1;
        }
        else if (!hayActual[b] && !hayProx[b]) {
            // OFF -> OFF
            Banco_ApagarCeldas(&ctrl.bancos[b]);
            Banco_EncenderSwitch(&ctrl.bancos[b]);
        }
        else {
            // OFF -> ON
            Banco_ApagarSwitch(&ctrl.bancos[b]);
            Banco_ApagarCeldas(&ctrl.bancos[b]);
            necesitaPaso2 = 1;
        }
    }

    Controlador_AplicarEstados();

    if (!necesitaPaso2) {
        return;
    }



    // 3. Preparo paso 2 para todos los bancos
    for (uint8_t b = 0; b < ctrl.cant_bancos; b++) {
        if (hayActual[b] && !hayProx[b]) {
            // ON -> OFF
            Banco_EncenderSwitch(&ctrl.bancos[b]);
            Banco_ApagarCeldas(&ctrl.bancos[b]);
        }
        else if (!hayActual[b] && hayProx[b]) {
            // OFF -> ON
            Banco_ApagarSwitch(&ctrl.bancos[b]);
            Banco_SetPatronCeldas(&ctrl.bancos[b], patron[b]);
        }
    }

    Controlador_AplicarEstados();
}



void Controlador_Tick1ms(void)
{
    if (periodo_ms == 0) {
        return;
    }

    contador++;

    if (contador >= periodo_ms) {
        contador = 0;
        fase ^= 1;
    }
}

static uint8_t Controlador_GetModoCelda(char modo)
{
	switch (modo) {
	case 's': return SYNCHRO;
	case 'c': return COMPLEMENTARY;
	default: return FIJO;
	}
}


resultado_t Controlador_IniciarSwitchingCelda(uint8_t banco, uint8_t celda){
    resultado_t validacion = Controlador_ValidarOperacion();
    if (validacion != RES_OK) RECHAZAR(validacion, ctrl.estado);

	// CAMBIAR ESTO, QUE NO ENTRE DIRECTO A FREQ
    if (verificar_banco(banco) == -1) RECHAZAR(ERR_BANCO, banco);
    if (celda >= ctrl.bancos[banco].cant_celdas) RECHAZAR(ERR_CELDA, celda);
	if (periodo_ms == 0){
		periodo_ms = PERIODO_DEFAULT;
		contador = 0;
		fase = 0;
	}

	Banco_SetModoCelda(&ctrl.bancos[banco], celda, SYNCHRO);
    if (ctrl.estado == ABIERTO) ctrl.estado = REPOSO;
    return RES_OK;
}

resultado_t Controlador_ModificarModoCelda(uint8_t banco, uint8_t celda, char modo){
    if (modo != 's' && modo != 'c' && modo != 'f') RECHAZAR(ERR_MODO, (uint8_t)modo);

    resultado_t validacion = Controlador_ValidarOperacion();
    if (validacion != RES_OK) RECHAZAR(validacion, ctrl.estado);

    if (verificar_banco(banco) == -1) RECHAZAR(ERR_BANCO, banco);
    if (celda >= ctrl.bancos[banco].cant_celdas) RECHAZAR(ERR_CELDA, celda);
	Banco_SetModoCelda(&ctrl.bancos[banco], celda, Controlador_GetModoCelda(modo));
    if (ctrl.estado == ABIERTO) ctrl.estado = REPOSO;
    return RES_OK;
}


resultado_t Controlador_ModificarModo(uint8_t banco, char modo){
    if (modo != 's' && modo != 'c' && modo != 'f') RECHAZAR(ERR_MODO, (uint8_t)modo);

    resultado_t validacion = Controlador_ValidarOperacion();
    if (validacion != RES_OK) RECHAZAR(validacion, ctrl.estado);

    if (verificar_banco(banco) == -1) RECHAZAR(ERR_BANCO, banco);
	Banco_SetModo(&ctrl.bancos[banco], Controlador_GetModoCelda(modo));
    if (ctrl.estado == ABIERTO) ctrl.estado = REPOSO;
    return RES_OK;
}

resultado_t Controlador_ModificarPeriodo(uint16_t periodo)
{
    if (periodo < PERIODO_DEFAULT) RECHAZAR(ERR_PERIODO, periodo);

    resultado_t validacion = Controlador_ValidarOperacion();
    if (validacion != RES_OK) RECHAZAR(validacion, ctrl.estado);

    periodo_ms = periodo;
    contador = 0;
    return RES_OK;
}


resultado_t Controlador_PararSwitchingCelda(uint8_t banco, uint8_t celda){
    resultado_t validacion = Controlador_ValidarOperacion();
    if (validacion != RES_OK) RECHAZAR(validacion, ctrl.estado);

	 if (verificar_banco(banco) == -1) RECHAZAR(ERR_BANCO, banco);
    if (celda >= ctrl.bancos[banco].cant_celdas) RECHAZAR(ERR_CELDA, celda);

	 Banco_DetenerSwitchingCelda(&ctrl.bancos[banco], celda);

    if (ctrl.estado == ABIERTO) ctrl.estado = REPOSO;
    return RES_OK;
}


resultado_t Controlador_DetenerSwitchingBancoBypass(uint8_t banco)
{
    resultado_t validacion = Controlador_ValidarOperacion();
    if (validacion != RES_OK) RECHAZAR(validacion, ctrl.estado);

    if (verificar_banco(banco) == -1) RECHAZAR(ERR_BANCO, banco);

    Banco_SetModo(&ctrl.bancos[banco], FIJO);




    if (ctrl.estado == ABIERTO) ctrl.estado = REPOSO;
    return RES_OK;
}

resultado_t Controlador_IniciarSwitchingBanco(uint8_t banco){
    resultado_t validacion = Controlador_ValidarOperacion();
    if (validacion != RES_OK) RECHAZAR(validacion, ctrl.estado);

    if (verificar_banco(banco) == -1) RECHAZAR(ERR_BANCO, banco);
	for (uint8_t c = 0; c < ctrl.bancos[banco].cant_celdas; c++){
		Controlador_IniciarSwitchingCelda(banco, c);
	}
    if (ctrl.estado == ABIERTO) ctrl.estado = REPOSO;
    return RES_OK;
}




resultado_t Controlador_EstadoAbierto(void){
    resultado_t validacion = Controlador_ValidarOperacion();
    if (validacion != RES_OK) RECHAZAR(validacion, ctrl.estado);

    Controlador_CancelarBarrido();
    periodo_ms = contador = fase = 0;
    for (uint8_t b = 0; b < ctrl.cant_bancos; b++) Banco_SetModo(&ctrl.bancos[b], FIJO);
	for (uint8_t i = 0; i < ctrl.cant_bancos; i++){
		Banco_ApagarSwitch(&ctrl.bancos[i]);
		Banco_ApagarCeldas(&ctrl.bancos[i]);
		Controlador_AplicarEstados();
	}
	ctrl.estado = ABIERTO;
    return RES_OK;
}

static void SeleccionarMux(uint8_t celda){
    MUX_Select(&mux_bancos[0], celda);
    for (uint8_t b = 0; b < CANT_BANCOS; b++) {
        mux_bancos[b].canal_seleccionado = celda;
    }
}



uint16_t Controlador_MedirCellNeg(uint8_t banco){
    if (verificar_banco(banco) == -1) return 0;
	return adc_get_node_voltage_mV(mux_bancos[banco].nodo);
}


void Controlador_CargarMediciones(void){
	for (uint8_t i = 0; i < ADC_NODE_COUNT; i++){
		ctrl.mediciones_anteriores[i] = ctrl.mediciones[i];
	}
	adc_get_voltages_mV(ctrl.mediciones,ADC_NODE_COUNT);
}

uint16_t Controlador_GetMedicion(adc_node_t nodo)
{
    if (nodo >= ADC_NODE_COUNT)
    {
        return 0u;
    }

    return ctrl.mediciones[nodo];
}


uint16_t Controlador_GetTensionSalida_mV(void){
	uint16_t total = 0;
	for (int b = 0; b < ctrl.cant_bancos; b++){
		if (ctrl.bancos[b].actual == ON ||
		    !Banco_HayCeldasActualON(&ctrl.bancos[b])) continue;
		total += Controlador_GetTensionBanco_mV(b);
	}
	return total;
}

uint16_t Controlador_GetTensionBanco_mV(uint8_t banco)
{
    if (verificar_banco(banco) == -1) return 0u;
    return adc_get_tension_banco_mV(banco);
}

uint16_t Controlador_GetCellNeg_mV(uint8_t banco)
{
    if (verificar_banco(banco) == -1) return 0u;
    return adc_get_cell_neg_mV(banco);
}

uint16_t Controlador_GetCorrienteDescarga_mA(void)
{
    return adc_get_corriente_descarga_mA();
}

uint16_t Controlador_GetCorrienteCarga_mA(void)
{
    return adc_get_corriente_carga_mA();
}


uint8_t Controlador_IniciarBarrido(void)
{
	if (ctrl.estado == PRUEBA) {
	    Diagnostico_Registrar(ERR_ESTADO, __func__, ctrl.estado);
	    return 0;
	}
    resultado_t error = ctrl.cant_bancos == 0 ? ERR_NO_INICIALIZADO :
        (ctrl.estado == INIT || ctrl.estado == FALLA) ? ERR_ESTADO :
        estado_barrido == BARRIDO_EN_CURSO ? ERR_OCUPADO :
        !adc_is_dma_started() ? ERR_ADC_INACTIVO : RES_OK;
    if (error != RES_OK) {
        Diagnostico_Registrar(error, __func__, ctrl.estado);
        return 0u;
    }
    memset(mediciones_celdas, 0, sizeof(mediciones_celdas));
    estado_barrido = BARRIDO_EN_CURSO;
    barrido_celda = 0u;
    barrido_espera_adc = 0u;
    SeleccionarMux(barrido_celda);
    barrido_inicio_ms = barrido_seleccion_ms = HAL_GetTick();
    barrido_salidas_version = salidas_version;
    return 1u;
}

void Controlador_CancelarBarrido(void)
{
    if (estado_barrido == BARRIDO_EN_CURSO) {
        estado_barrido = BARRIDO_CANCELADO;
        Diagnostico_Registrar(ADV_BARRIDO_CANCELADO, __func__, barrido_celda);
    }
}

barrido_estado_t Controlador_GetEstadoBarrido(void)
{
    return estado_barrido;
}

uint8_t Controlador_GetMedicionCelda(uint8_t banco, uint8_t celda,
                                    medicion_celda_t *medicion)
{
    if (medicion == NULL || banco >= ctrl.cant_bancos ||
        celda >= ctrl.bancos[banco].cant_celdas) {
        Diagnostico_Registrar(medicion == NULL ? ERR_PUNTERO :
                             banco >= ctrl.cant_bancos ? ERR_BANCO : ERR_CELDA,
                             __func__, ((uint32_t)banco << 8) | celda);
        return 0u;
    }
    *medicion = mediciones_celdas[banco][celda];
    return medicion->valida;
}

void Controlador_ActualizarBarrido(void)
{
	if (ctrl.estado == PRUEBA) return;
    if (estado_barrido != BARRIDO_EN_CURSO) return;
    if (ctrl.estado == INIT || ctrl.estado == FALLA) {
        Controlador_CancelarBarrido();
        return;
    }
    uint32_t ahora = HAL_GetTick();
    if ((uint32_t)(ahora - barrido_inicio_ms) >= BARRIDO_TIMEOUT_MS) {
        estado_barrido = BARRIDO_TIMEOUT;
        Diagnostico_Registrar(ERR_BARRIDO_TIMEOUT, __func__, barrido_celda);
        return;
    }
    if (salidas_version != barrido_salidas_version) {
        /* Una conmutacion puede contaminar el bloque: repetir esta posicion. */
        barrido_salidas_version = salidas_version;
        barrido_seleccion_ms = ahora;
        barrido_espera_adc = 0u;
    }
    if (!barrido_espera_adc) {
        if ((uint32_t)(ahora - barrido_seleccion_ms) < MUX_ASENTAMIENTO_MS) return;
        barrido_adc_marca = adc_get_conversion_sequence();
        barrido_espera_adc = 1u;
        return;
    }
    /* Descartar la mitad en curso al terminar el asentamiento; la siguiente
     * estara formada integramente por muestras de esta seleccion. */
    int32_t bloques = (int32_t)(adc_get_processed_sequence() - barrido_adc_marca);
    if (!adc_has_measurements() || bloques < 2) return;

    for (uint8_t b = 0; b < ctrl.cant_bancos; b++) {
        medicion_celda_t *m = &mediciones_celdas[b][barrido_celda];
        uint16_t raw = adc_get_node_raw((adc_node_t)(ADC_MUX_BANCO_1 + b));
        m->cell_neg_mV = adc_get_cell_neg_mV(b);
        m->banco_mV = adc_get_tension_banco_mV(b);
        m->instante_ms = adc_get_processed_ms();
        m->limite_adc = (raw == 0u || raw == 4095u);
        if (m->limite_adc) Diagnostico_Registrar(ADV_LIMITE_ADC, __func__,
                                               ((uint32_t)(b + 1u) << 8) | (barrido_celda + 1u));
        m->celda_on = ctrl.bancos[b].celdas[barrido_celda].actual;
        m->bypass_on = ctrl.bancos[b].actual;
        m->valida = 1u;
    }
    barrido_celda++;
    if (barrido_celda >= Controlador_GetCantCeldas()) {
        estado_barrido = BARRIDO_COMPLETO;
        return;
    }
    SeleccionarMux(barrido_celda);
    barrido_seleccion_ms = ahora;
    barrido_espera_adc = 0u;
}

resultado_t Controlador_SeleccionarMux(uint8_t celda)
{
    if (ctrl.cant_bancos == 0) RECHAZAR(ERR_NO_INICIALIZADO, 0u);
    if (celda >= Controlador_GetCantCeldas()) RECHAZAR(ERR_CELDA, celda);
    Controlador_CancelarBarrido();
    SeleccionarMux(celda);
    return RES_OK;
}
