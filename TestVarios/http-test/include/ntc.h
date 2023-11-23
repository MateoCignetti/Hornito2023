#ifndef NTC_H
#define NTC_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "adc.h"

float get_ntc_temperature_c(uint16_t adc_voltage_mv);
void create_ntc_tasks();

#endif