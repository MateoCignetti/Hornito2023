#ifndef POWER_H
#define POWER_H

#include "adc.h"
#include "esp_timer.h"
#include "math.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

void create_sampling_timer();
void setup_power_isr();
void create_power_tasks();

extern QueueHandle_t xQueuePower;
extern SemaphoreHandle_t xSemaphorePower;

#endif