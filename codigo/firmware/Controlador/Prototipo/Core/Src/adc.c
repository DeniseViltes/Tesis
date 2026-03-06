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

uint16_t g_adc_raw[ADC_MEAS_CH_COUNT] = {0u};
static uint8_t g_adc_dma_started = 0u;

/********************** internal data definition *****************************/

/********************** internal functions definitions ***********************/



/********************** internal functions declaration ***********************/

void adc_init(void)
{
	if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)g_adc_raw, ADC_MEAS_CH_COUNT) == HAL_OK)
	  {
	    g_adc_dma_started = 1u;
	  }
}



void adc_update(void)
{
  /* El modo DMA se encarga de actualizar g_adc_raw[]  */
}

uint16_t adc_get_raw(adc_meas_ch_t channel)
{
  if ((uint32_t)channel >= ADC_MEAS_CH_COUNT)
  {
    return 0u;
  }

  return g_adc_raw[(uint32_t)channel];
}

const uint16_t *adc_get_raw_buffer(void)
{
  return g_adc_raw;
}

uint8_t adc_is_dma_started(void)
{
  return g_adc_dma_started;
}
