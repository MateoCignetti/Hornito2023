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

// Function to create the control system tasks
void create_control_system_tasks();

// Function to create the control system semaphores and queues
void create_control_system_semaphores_queues();

// Function to delete the control system tasks
void delete_control_system_tasks();

extern QueueHandle_t xQueueControlSystem;                   // Queue to send temperature to web
extern SemaphoreHandle_t xSemaphoreControlSystem;           // Semaphore to indicate that temperature is ready to send
extern SemaphoreHandle_t xSemaphoreControlSystemToPower;    // Semaphore to indicate that steps are ready to send
extern QueueHandle_t xQueueControlSystemToPower;            // Queue to send steps to power

#endif