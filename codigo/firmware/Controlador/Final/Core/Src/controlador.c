/*
 * controlador.c
 *
 *  Created on: 10 jun 2026
 *      Author: ---
 */

#include "controlador.h"
#include "main.h"
#include "llave.h"
#include "procesar_adc.h"
#include "fallas.h"

static controlador_t ctrl;

extern volatile uint8_t flag_controlador_update;


static shift_register_t sr_bancos[CANT_BANCOS];
static mux_t mux_bancos;



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



static int verificar_banco(uint8_t banco);

static void Controlador_EnviarCeros(void){
    for (uint8_t b = 0; b < CANT_BANCOS; b++) {
        ctrl.bancos[b].prox = OFF;
        for (uint8_t c = 0; c < CELDAS_POR_BANCO; c++)
            ctrl.bancos[b].celdas[c].prox = OFF;
    }
    for (int8_t pin = CANT_PINES_SR - 1; pin >= 0; pin--) {
        for (uint8_t b = 0; b < CANT_BANCOS; b++)
            Banco_AplicarEstadoPin(&ctrl.bancos[b], pin);
        Banco_ClockPulse(&ctrl.bancos[0]);
    }
    Banco_LatchPulse(&ctrl.bancos[0]);
}


static int puede_operar(void){
    return ctrl.cant_bancos != 0 &&
           (ctrl.estado == ACTIVO || ctrl.estado == PRUEBA);
}


int Controlador_init(uint8_t bancos, uint8_t celdas){

	if (bancos == 0 || bancos > CANT_BANCOS ||
	    celdas == 0 || celdas > CELDAS_POR_BANCO) {
	    return 1;
	}
	if (ctrl.cant_bancos != 0 &&
	    (ctrl.estado == ACTIVO || ctrl.estado == PRUEBA)) {
	    Controlador_EnviarCeros();
	}
    Llave_Init();
    periodo_ms = contador = fase = 0;
    ctrl = (controlador_t){0};
	ctrl.cant_bancos = bancos;

	MUX_Init(&mux_bancos, S2_GPIO_Port, S2_Pin,
			         S1_GPIO_Port, S1_Pin, S0_GPIO_Port, S0_Pin);

	for (uint8_t i = 0; i < CANT_BANCOS; i++){
		ctrl.bancos[i].id=i;

		SR_Init(&sr_bancos[i],sr_data_puertos[i],
				sr_data_pines[i],CLK_GPIO_Port,
				CLK_Pin,LATCH_GPIO_Port, LATCH_Pin);



		Banco_Init(
		    &ctrl.bancos[i],
		    &sr_bancos[i],
		    &mux_bancos,
		    i < bancos ? celdas : 0,
		    ADC_MUX_BANCO_1 + i
		);

	}

    ctrl.estado = REPOSO;
    ctrl.rl = 0;
    ctrl.modo = MODO_SIN_ASIGNAR;
	return 0;
}


int Controlador_ActivarCircuito(controlador_modo_t modo){


    if (ctrl.cant_bancos == 0 || ctrl.estado != REPOSO) {
        return 1;
    }
    if (modo != MODO_CARGA && modo != MODO_DESCARGA) {
        return 1;
    }
    if (modo == MODO_DESCARGA && ctrl.rl == 0) {
        return 1;
    }
    ctrl.estado = ACTIVO;
    ctrl.modo = modo;
	Llave_Habilitar();
	HAL_Delay(TURN_ON_DELAY);
	Controlador_EnviarCeros();

	for (uint8_t b = 0; b < ctrl.cant_bancos; b++)
	    Banco_EncenderSwitch(&ctrl.bancos[b]);

	Controlador_AplicarEstados();

	return 0;
}


int Controlador_ModoPrueba(void){

	if (ctrl.cant_bancos == 0 || ctrl.estado != REPOSO) {
	        return 1;
	    }

	Llave_Habilitar();
		HAL_Delay(TURN_ON_DELAY);
		Controlador_EnviarCeros();

		for (uint8_t b = 0; b < ctrl.cant_bancos; b++)
		    Banco_EncenderSwitch(&ctrl.bancos[b]);

		Controlador_AplicarEstados();
		ctrl.estado = PRUEBA;
		ctrl.modo = MODO_SIN_ASIGNAR;
	return 0;
}



int Controlador_EvaluarFallas(void){
	if (ctrl.estado == FALLA) return 1;
	if (ctrl.estado != ACTIVO) return 0;
	if (!puede_operar()) return 1;
	if (!adc_is_dma_started() || !adc_has_measurements()) return 0; //TODO

    if (Fallas_EvaluarCorrientes(
        Controlador_GetCorrienteDescarga(),
        Controlador_GetCorrienteCarga()
    ) != FALLA_NINGUNA) return 1;


    for (uint8_t b = 0; b < ctrl.cant_bancos; b++) {
            if (ctrl.bancos[b].actual == ON ||
                !Banco_HayCeldasActualON(&ctrl.bancos[b])) continue;

            /* Aproximacion por tension del banco activo, no un diagnostico
             * de una celda individual. */
            if (Fallas_EvaluarTensionCelda(Controlador_GetTensionBanco(b))
                != FALLA_NINGUNA) return 1;
        }

    return 0;
}

void Controlador_Update(void){
	if (ctrl.estado == REPOSO || ctrl.estado == FALLA  ) return;


	if (Controlador_EvaluarFallas()) {
	    periodo_ms = contador = fase = 0;
	    Controlador_EnviarCeros();
	    Llave_DispararFalla();
	    ctrl.estado = FALLA;
	    return;
	}

    Controlador_Tick1ms();
    Controlador_ActualizarEstados();

}

status_contraldor_t Controlador_GetEstado(void){
	return ctrl.estado;
}
uint8_t Controlador_GetCantBancos(void){
	return ctrl.cant_bancos;
}


uint8_t Controlador_GetCantCeldas(void){
    return ctrl.cant_bancos ? ctrl.bancos[0].cant_celdas : 0;
}

int Controlador_SetResistenciaRL(uint16_t resistencia){
	if (ctrl.estado != REPOSO) return 1;
	if (ctrl.cant_bancos == 0) return 1;
	if (resistencia == 0 || resistencia > 1000) return 1;

	ctrl.rl = resistencia;
	return 0;
}


uint16_t Controlador_GetResistenciaRL(void){
    return ctrl.rl;
}

static void Controlador_ClockPulse(void){
	Banco_ClockPulse(&ctrl.bancos[0]);
}


static void Controlador_LatchPulse(void){
	Banco_LatchPulse(&ctrl.bancos[0]);
}



uint8_t Controlador_GetEstadoBanco(uint8_t banco){
    if (!verificar_banco(banco)) return OFF;

	return (uint8_t) Banco_GetEstado(&ctrl.bancos[banco]);
}

uint8_t Controlador_GetEstadoCelda(uint8_t banco, uint8_t celda){
    if (!verificar_banco(banco) || celda >= ctrl.bancos[banco].cant_celdas) return OFF;

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
}




int Controlador_HayCambios(void){
    for (uint8_t i = 0; i < CANT_BANCOS; i++) {
        if (Banco_HayCambios(&ctrl.bancos[i])) {
            return 1;
        }
    }

    return 0;
}


// Control individual




int Controlador_EncenderCelda(uint8_t banco, uint8_t celda)
{
    if (!puede_operar() || !verificar_banco(banco) || celda >= ctrl.bancos[banco].cant_celdas) return 1;


    // Paso 1: solo modifico el banco objetivo.
    // Los otros bancos quedan con su prox intacto.
    if (Banco_GetEstadoActual (&ctrl.bancos[banco]) == ON){
		Banco_ApagarSwitch(&ctrl.bancos[banco]); // sw banco apagado
		Controlador_AplicarEstados(); // escribe TODOS los bancos y hace latch comun



    }


    // Paso 2: prendo la celda.
    Banco_EncenderCelda(&ctrl.bancos[banco], celda);
    Controlador_AplicarEstados(); // otra vez frame completo + latch comun
    return 0;
}



int Controlador_ApagarCelda(uint8_t banco, uint8_t celda)
{
    if (!puede_operar() || !verificar_banco(banco) || celda >= ctrl.bancos[banco].cant_celdas) return 1;



    //Paso 1: Apago la celda
    Banco_ApagarCelda(&ctrl.bancos[banco],celda);
    Controlador_AplicarEstados(); // escribe TODOS los bancos y hace latch comun



    if (!Banco_HayCeldasProxON(&ctrl.bancos[banco])){ //no quedan celdas en ON->enciendo el banco



    	// Paso 2: prendo la celda.
    	Banco_EncenderSwitch(&ctrl.bancos[banco]);
    	Controlador_AplicarEstados(); // otra vez frame completo + latch comun

    }
    return 0;
}




//Control por bancos

int verificar_banco(uint8_t banco){
	if (banco >= ctrl.cant_bancos ) return 0;
	return 1;
}

int Controlador_BypassBanco(uint8_t banco){
	if (!puede_operar() || !verificar_banco(banco)) return 1;

	Banco_ApagarSwitch(&ctrl.bancos[banco]);//solo or si acaso
	Banco_ApagarCeldas(&ctrl.bancos[banco]);
	Controlador_AplicarEstados();



	Banco_EncenderSwitch(&ctrl.bancos[banco]);
    Controlador_AplicarEstados();
    return 0;
}

int Controlador_ActivarCeldasBanco(uint8_t banco){
	if (!puede_operar() || !verificar_banco(banco)) return 1;

	Banco_ApagarSwitch(&ctrl.bancos[banco]);
	Controlador_AplicarEstados();



	Banco_EncenderCeldas(&ctrl.bancos[banco]);
	Controlador_AplicarEstados();
    return 0;
}



/*
 * Sirve  para actualizar estados con y sin switching
 */
void Controlador_ActualizarEstados(void)
{
    uint8_t patron[CANT_BANCOS] = {0};
    uint8_t hayActual[CANT_BANCOS] = {0};
    uint8_t hayProx[CANT_BANCOS] = {0};
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


int Controlador_IniciarSwitchingCelda(uint8_t banco, uint8_t celda){
	// CAMBIAR ESTO, QUE NO ENTRE DIRECTO A FREQ
    if (!puede_operar() || !verificar_banco(banco)) return 1;
    if (celda >= ctrl.bancos[banco].cant_celdas) return 1;
	if (periodo_ms == 0){
		periodo_ms = PERIODO_DEFAULT;
		contador = 0;
		fase = 0;
	}

	Banco_SetModoCelda(&ctrl.bancos[banco], celda, SYNCHRO);
    return 0;
}

int Controlador_ModificarModoCelda(uint8_t banco, uint8_t celda, char modo){
    if (modo != 's' && modo != 'c') return 1;

    if (!puede_operar() || !verificar_banco(banco) || celda >= ctrl.bancos[banco].cant_celdas) return 1;

	Banco_SetModoCelda(&ctrl.bancos[banco], celda, Controlador_GetModoCelda(modo));
    return 0;
}


int Controlador_ModificarModo(uint8_t banco, char modo){
    if (modo != 's' && modo != 'c') return 1;

    if (!puede_operar() || !verificar_banco(banco)) return 1;

	Banco_SetModo(&ctrl.bancos[banco], Controlador_GetModoCelda(modo));
    return 0;
}

int Controlador_ModificarPeriodo(uint16_t periodo)
{
    if (!puede_operar() || periodo < PERIODO_DEFAULT) return 1;

    periodo_ms = periodo;
    contador = 0;
    return 0;
}


int Controlador_PararSwitchingCelda(uint8_t banco, uint8_t celda){
    if (!puede_operar() || !verificar_banco(banco) || celda >= ctrl.bancos[banco].cant_celdas) return 1;




	 Banco_DetenerSwitchingCelda(&ctrl.bancos[banco], celda);
    return 0;
}


int Controlador_DetenerSwitchingBancoBypass(uint8_t banco)
{
    if (!puede_operar() || !verificar_banco(banco)) return 1;

    Banco_SetModo(&ctrl.bancos[banco], FIJO);
    return 0;
}

int Controlador_IniciarSwitchingBanco(uint8_t banco){
    if (!puede_operar() || !verificar_banco(banco)) return 1;

	for (uint8_t c = 0; c < ctrl.bancos[banco].cant_celdas; c++){
		Banco_SetModoCelda(&ctrl.bancos[banco], c, SYNCHRO);
	}
    return 0;
}





int Controlador_SeleccionarMux(uint8_t celda){
	if (!puede_operar() || celda >= Controlador_GetCantCeldas()) {
	        return 1;
	    }

	Banco_SeleccionarCeldaMux(&ctrl.bancos[0], celda);
	return 0;
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

uint16_t Controlador_GetTensionSalida(void){
	uint16_t total = 0;
	for (int b = 0; b < ctrl.cant_bancos; b++){
		if (ctrl.bancos[b].actual == ON ||
		    !Banco_HayCeldasActualON(&ctrl.bancos[b])) continue;
		total += Controlador_GetTensionBanco(b);
	}
	return total;
}


uint16_t Controlador_GetTensionBanco(uint8_t banco)
{
    if (!verificar_banco(banco)) return 0u;
    return adc_get_tension_banco_mV(banco);
}

uint16_t Controlador_GetCellNeg(uint8_t banco)
{
    if (!verificar_banco(banco)) return 0u;
    return adc_get_cell_neg_mV(banco);
}


uint16_t Controlador_GetCorrienteDescarga(void)
{
    return adc_get_corriente_descarga_mA();
}


uint16_t Controlador_GetCorrienteCarga(void)
{
    return adc_get_corriente_carga_mA();
}



