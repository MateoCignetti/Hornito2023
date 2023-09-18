// Includes

#include "adc.h"
#include "dimmer.h"
//

// Defines

//

// Typedef

//

// ESP-LOG tasks

//

// Handles

//

// Global variables

//

// Function prototypes
void tasks_create();
//

// Main
void app_main(void){
    adc1_init();
    setup_dimmer_isr();
    tasks_create();
}
//

// Functions

//

// FreeRTOS Task create
void tasks_create(){
    create_adc_read_task();
    create_dimmer_task();
}
//

