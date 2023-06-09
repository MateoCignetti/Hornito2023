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

#define PIN_IN GPIO_NUM_35
#define PIN_OUT GPIO_NUM_32

void app_main(void){
    gpio_set_direction(PIN_OUT, GPIO_MODE_OUTPUT);
    
    gpio_set_direction(PIN_IN, GPIO_MODE_INPUT);
    gpio_set_intr_type(PIN_IN, GPIO_INTR_ANYEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_IN, )
}