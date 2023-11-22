#ifndef ADC_H
#define ADC_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include <math.h>

void adc_init(); // Initializes ADC, see the defines in adc.c for the default values and change accordingly
int get_adc_voltage_mv(adc_unit_t ADC_UNIT, adc_channel_t ADC_CHANNEL); // Returns the voltage in mV from the ADC
int get_adc_voltage_mv_multisampling(adc_unit_t ADC_UNIT, adc_channel_t ADC_CHANNEL); // Returns the voltage in mV from the ADC with multisampling, see the defines in adc.c for the default values and change accordingly
void create_adc_tasks(); // Creates the ADC tasks, see adc.c

#endif