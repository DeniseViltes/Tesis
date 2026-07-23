/*
 * controlador.c
 *
 *  Created on: 10 jun 2026
 *      Author: ---
 */

#include "controlador.h"
#include "main.h"


#define DEADTIME 0

static controlador_t ctrl;

extern volatile uint8_t flag_controlador_update;


static shift_register_t sr_bancos[CANT_BANCOS];
static mux_t mux_bancos[CANT_BANCOS];



static uint16_t periodo_ms = 0;
static uint16_t contador = 0;
static uint8_t fase = 0;


//por ahora solo tengo el banco 1
static GPIO_TypeDef *sr_data_puertos[CANT_BANCOS] = {
	DATA1_GPIO_Port,
	DATA2_GPIO_Port,
};

static uint16_t sr_data_pines[CANT_BANCOS] = {
	DATA1_Pin,
	DATA2_Pin,
};




int verificar_banco(uint8_t banco);

void Controlador_init(void){



	for (uint8_t i = 0; i < CANT_BANCOS; i++){
		ctrl.bancos[i].id=i;

		SR_Init(&sr_bancos[i],sr_data_puertos[i],sr_data_pines[i],CLK_GPIO_Port, CLK_Pin,LATCH_GPIO_Port, LATCH_Pin);

		MUX_Init(&mux_bancos[i], S0_GPIO_Port, S0_Pin,S1_GPIO_Port, S1_Pin,S2_GPIO_Port,S2_Pin);

		Banco_Init(&ctrl.bancos[i],&sr_bancos[i],&mux_bancos[i],CELDAS_POR_BANCO, ADC_MUX_BANCO_0);


		Controlador_BypassBanco(i);
	}
	//harcodeo esto por ahora.
	ctrl.cant_bancos = 1;
	//apagar_banco

	//Controlador_AplicarEstados();
}


void Controlador_Update(void){
	Controlador_Tick1ms();
	Controlador_ActualizarEstados();

}



static void Controlador_ClockPulse(void){
	Banco_ClockPulse(&ctrl.bancos[0]);
}


static void Controlador_LatchPulse(void){
	Banco_LatchPulse(&ctrl.bancos[0]);
}



uint8_t Controlador_GetEstadoBanco(uint8_t banco){
	return (uint8_t) Banco_GetEstado(&ctrl.bancos[banco]);
}

uint8_t Controlador_GetEstadoCelda(uint8_t banco, uint8_t celda){
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




void Controlador_EncenderCelda(uint8_t banco, uint8_t celda)
{
    if (verificar_banco(banco) == -1) return;


    // Paso 1: solo modifico el banco objetivo.
    // Los otros bancos quedan con su prox intacto.
    if (Banco_GetEstadoActual (&ctrl.bancos[banco]) == ON){
		Banco_ApagarSwitch(&ctrl.bancos[banco]); // sw banco apagado
		Controlador_AplicarEstados(); // escribe TODOS los bancos y hace latch comun


		HAL_Delay(DEADTIME);
    }


    // Paso 2: prendo la celda.
    Banco_EncenderCelda(&ctrl.bancos[banco], celda);
    Controlador_AplicarEstados(); // otra vez frame completo + latch comun
}



void Controlador_ApagarCelda(uint8_t banco, uint8_t celda)
{
    if (verificar_banco(banco) == -1) return;

    //Paso 1: Apago la celda
    Banco_ApagarCelda(&ctrl.bancos[banco],celda);
    Controlador_AplicarEstados(); // escribe TODOS los bancos y hace latch comun



    if (!Banco_HayCeldasProxON(&ctrl.bancos[banco])){ //no quedan celdas en ON->enciendo el banco

    	HAL_Delay(DEADTIME);

    	// Paso 2: prendo la celda.
    	Banco_EncenderSwitch(&ctrl.bancos[banco]);
    	Controlador_AplicarEstados(); // otra vez frame completo + latch comun

    }
}




//Control por bancos

int verificar_banco(uint8_t banco){
	if (banco >= CANT_BANCOS || banco < 0 )
		return -1;
	return 1;
}

void Controlador_BypassBanco(uint8_t banco){
	if (verificar_banco(banco) == -1) return;

	Banco_ApagarSwitch(&ctrl.bancos[banco]);//solo or si acaso
	Banco_ApagarCeldas(&ctrl.bancos[banco]);
	Controlador_AplicarEstados();

	HAL_Delay(DEADTIME);

	Banco_EncenderSwitch(&ctrl.bancos[banco]);
    Controlador_AplicarEstados();

}

void Controlador_ActivarCeldasBanco(uint8_t banco){
	if (verificar_banco(banco) == -1) return;

	Banco_ApagarSwitch(&ctrl.bancos[banco]);
	Controlador_AplicarEstados();

	HAL_Delay(DEADTIME);

	Banco_EncenderCeldas(&ctrl.bancos[banco]);
	Controlador_AplicarEstados();
}



/*
 * Sirve  para actualizar estados con y sin switching
 */
void Controlador_ActualizarEstados(void)
{
    uint8_t patron[CANT_BANCOS];
    uint8_t hayActual[CANT_BANCOS];
    uint8_t hayProx[CANT_BANCOS];
    uint8_t necesitaPaso2 = 0;

    //if (Controlador_HayCambios() == 0 )	return;
    // 1. Calculo todo primero
    for (uint8_t b = 0; b < CANT_BANCOS; b++) {

        patron[b] = Banco_CalcularPatron(&ctrl.bancos[b],fase);
        hayActual[b] = Banco_HayCeldasActualON(&ctrl.bancos[b]);
        hayProx[b] = (patron[b] != 0);
    }

    // 2. Preparo paso 1 para todos los bancos
    for (uint8_t b = 0; b < CANT_BANCOS; b++) {
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

    HAL_Delay(DEADTIME);

    // 3. Preparo paso 2 para todos los bancos
    for (uint8_t b = 0; b < CANT_BANCOS; b++) {
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


void Controlador_IniciarSwitchingCelda(uint8_t banco, uint8_t celda){
	// CAMBIAR ESTO, QUE NO ENTRE DIRECTO A FREQ
    if (verificar_banco(banco) == -1) return;
    if (celda >= CELDAS_POR_BANCO) return;
	if (periodo_ms == 0){
		periodo_ms = PERIODO_DEFAULT;
		contador = 0;
		fase = 0;
	}
	for (int i = 0; i < CANT_BANCOS; i++){
		fase = 1;
	}
	Banco_SetModoCelda(&ctrl.bancos[banco], celda, SYNCHRO);
}

void Controlador_ModificarModoCelda(uint8_t banco, uint8_t celda, char modo){
	Banco_SetModoCelda(&ctrl.bancos[banco], celda, Controlador_GetModoCelda(modo));
}


void Controlador_ModificarModo(uint8_t banco, char modo){
	Banco_SetModo(&ctrl.bancos[banco], Controlador_GetModoCelda(modo));
}

void Controlador_ModificarPeriodo(uint16_t periodo)
{
    periodo_ms = periodo;
    contador = 0;
}


void Controlador_PararSwitchingCelda(uint8_t banco, uint8_t celda){
	 if (verificar_banco(banco) == -1) return;

	 Banco_DetenerSwitchingCelda(&ctrl.bancos[banco], celda);
	 //Controlador_ActualizarEstados(); no hace falta poner al final, simpre se está actualizando
}


void Controlador_DetenerSwitchingBancoBypass(uint8_t banco)
{
    if (verificar_banco(banco) == -1) return;

    Banco_SetModo(&ctrl.bancos[banco], FIJO);



    //Controlador_ActualizarEstados();
}

void Controlador_IniciarSwitchingBanco(uint8_t banco){
	for (uint8_t c = 0; c < CELDAS_POR_BANCO; c++){
		Banco_SetModoCelda(&ctrl.bancos[banco], c, SYNCHRO);
	}
}




void Controlador_Reiniciar(void){
	for (uint8_t i = 0; i < CANT_BANCOS; i++){
		Banco_ApagarSwitch(&ctrl.bancos[i]);
		Banco_ApagarCeldas(&ctrl.bancos[i]);
		Controlador_AplicarEstados();
	}
}

void Controlador_CargarMediciones(void){
	uint16_t adc_mux [ADC_NODE_COUNT] = {0};
	adc_get_buffer(adc_mux,ADC_NODE_COUNT);
}
