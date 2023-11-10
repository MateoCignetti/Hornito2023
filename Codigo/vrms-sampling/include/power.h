#ifndef POWER_H
#define POWER_H

#include "adc.h"
#include "esp_timer.h"
#include "math.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"

void create_sampling_timer();
void setup_power_isr();
void create_power_tasks();

#endif