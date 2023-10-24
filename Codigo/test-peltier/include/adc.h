#ifndef ADC_H
#define ADC_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <math.h>

void adc1_init();
void adc1_create_oneshot_unit();
void adc1_channel0_config();
void adc1_calibration();
uint16_t get_adc1_c0_voltage_multisampling();
void create_adc_read_task();
void create_decision_peltier();
void vTaskAdc1C0Read();
void vTaskDecision();

#endif