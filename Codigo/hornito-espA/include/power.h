#ifndef POWER_H
#define POWER_H

#include "adc.h"
#include "control_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "math.h"

extern SemaphoreHandle_t xSemaphorePower;   // Semaphore to indicate that the power value should be read
extern QueueHandle_t xQueuePower;           // Queue for power readings

void create_power_tasks();                  // Create power-related FreeRTOS tasks
void create_power_semaphores_queues();      // Create power-related semaphores and queues
void delete_power_tasks();                  // Delete power-related FreeRTOS tasks

#endif