/* Project includes. */
#include "main.h"


/* App includes. */
#include "medicion.h"


/* Application includes. */


/********************** macros and definitions *******************************/


#define SAMPLES_COUNTER (100)
#define AVERAGER_SIZE (16)

/********************** external data declaration *****************************/

extern ADC_HandleTypeDef hadc1;

/********************** external functions definition ************************/



/********************** internal data declaration ****************************/
uint32_t tickstart;
uint16_t sample_idx;


uint16_t sample_array[SAMPLES_COUNTER];
bool b_trig_new_conversion;


/********************** internal data definition *****************************/

/********************** internal functions definitions ***********************/

bool tomar_medicion();
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef ADC_Poll_Read(uint16_t *value);

/********************** internal functions declaration ***********************/

void mediciones_init(void)
{

	HAL_NVIC_SetPriority(ADC1_2_IRQn, 2, 0);
	HAL_NVIC_EnableIRQ(ADC1_2_IRQn);

	sample_idx = 0;
	tickstart = HAL_GetTick();
}




void app_update(void)
{
	static bool b_test_done = false;

	b_test_done = test3_tick();

}



bool test3_tick() {

	bool b_done = false;

	if (sample_idx>=SAMPLES_COUNTER) {
		b_done = true;
		goto test3_tick_end;
	}

	/* start of first conversion */
	if (0==sample_idx) {
		b_trig_new_conversion = true;
	}


	if (b_trig_new_conversion) {
		b_trig_new_conversion = false;
		HAL_ADC_Start_IT(&hadc1);
	}

test3_tick_end:
	if (b_done) {
		for (sample_idx=0;sample_idx<SAMPLES_COUNTER;sample_idx++) {
			LOGGER_LOG("%u\n",sample_array[sample_idx] );
		}
	}
	return b_done;
}



void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {

	sample_array[sample_idx++] = HAL_ADC_GetValue(&hadc1);
	if (sample_idx<SAMPLES_COUNTER) {
		b_trig_new_conversion = true;
	}
}
