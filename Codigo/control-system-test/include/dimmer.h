#ifndef DIMMER_H
#define DIMMER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "esp_log.h"

void setup_dimmer_isr();
void create_dimmer_task();
void vTaskDimmer();
void set_dimmer_delay(int new_dimmer_delay_us);

#endif