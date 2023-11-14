#ifndef PELTIER_H
#define PELTIER_H

#include "driver/gpio.h"
#include "esp_log.h"
#include "adc.h"
#include "ntc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

extern SempahoreHandle_t xSemaphorePeltier;
extern QueueHandle_t xQueuePeltier;

void create_peltier_tasks();
void delete_peltier_tasks();

#endif