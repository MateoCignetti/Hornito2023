/**
 * @file main.c
 * @author Alejo, Facundo, Ignacio, Manuel, Mateo, Ramiro
 * @brief 
 * @version 0.1
 * @date 2023-06-08
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "driver/gpio.h"
#include "rom/ets_sys.h"

#define PIN_IN GPIO_NUM_4
#define PIN_OUT GPIO_NUM_2

#define DELAY_DIMMER_US 1000

static void isr_handler(void* arg);
static void dimmer_interrupt();

void app_main(void){
    gpio_set_direction(PIN_OUT, GPIO_MODE_OUTPUT);
    
    gpio_set_direction(PIN_IN, GPIO_MODE_INPUT);
    gpio_set_intr_type(PIN_IN, GPIO_INTR_ANYEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_IN, isr_handler, (void*) PIN_IN);
}

static void isr_handler(void* arg){
    uint32_t gpio_num = (uint32_t) arg;
    if(gpio_num == PIN_IN){
        dimmer_interrupt();
    }
}

static void dimmer_interrupt(){
    ets_delay_us(DELAY_DIMMER_US);
    gpio_set_level(PIN_OUT, 1);
    ets_delay_us(5);
    gpio_set_level(PIN_OUT, 0);
}