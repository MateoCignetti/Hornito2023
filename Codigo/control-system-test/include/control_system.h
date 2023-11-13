#ifndef CONTROL_SYSTEM_H
#define CONTROL_SYSTEM_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "esp_log.h"
#include "dimmer.h"
#include "ntc.h"

void create_control_system_tasks();

extern QueueHandle_t xQueueControlSystem;
extern SemaphoreHandle_t xSemaphoreControlSystem;
extern SemaphoreHandle_t xSemaphoreControlSystemToPower;
extern QueueHandle_t xQueueControlSystemToPower;

#endif