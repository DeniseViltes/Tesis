/* Project includes. */
#include <adc.h>
#include "main.h"


/* App includes. */


/* Application includes. */


/********************** macros and definitions *******************************/



/********************** external data declaration *****************************/

extern ADC_HandleTypeDef hadc1;


/********************** external functions definition ************************/



/********************** internal data declaration ****************************/



/********************** internal data definition *****************************/

/********************** internal functions definitions ***********************/



/********************** internal functions declaration ***********************/

void adc_init(void)
{
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)g_adc_raw, ADC_MEAS_CH_COUNT);
}




void adc_update(void)
{

}


