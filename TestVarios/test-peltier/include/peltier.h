#ifndef PELTIER_H
#define PELTIER_H

#include "driver/gpio.h"
#include "esp_log.h"
#include "adc.h"
#include "ntc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t xSemaphorePeltier; // Semaphore to indicate that temperature value must be sent
extern QueueHandle_t xQueuePeltier;         // Queue to send the temperature value

void create_peltier_tasks();                // Create peltier FreeRTOS tasks
void delete_peltier_tasks();                // Delete peltier FreeRTOS tasks
void create_peltier_semaphores_queues();    // Create peltier semaphores and queues  
#endif