#ifndef ADC_H
#define ADC_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include <math.h>

void adc1_init();
void adc2_init();
uint16_t get_adc_voltage_multisampling();
void create_adc_tasks();

#endif