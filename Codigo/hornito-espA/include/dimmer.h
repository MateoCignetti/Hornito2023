#ifndef DIMMER_H
#define DIMMER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "esp_log.h"

// Function to setup the dimmer ISR and semaphore. Should be called from main.c
void setup_dimmer_isr();

// Creates the dimmer task
void create_dimmer_task();

// Deletes the dimmer task
void delete_dimmer_task();

// Sets the dimmer delay in microseconds
void set_dimmer_delay(int new_dimmer_delay_us);

// Enables the dimmer
void enable_dimmer();

// Disables the dimmer
void disable_dimmer();

#endif