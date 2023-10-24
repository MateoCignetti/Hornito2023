// Includes

#include "adc.h"
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
    tasks_create();
}
//

// Functions

//

// FreeRTOS Task create
void tasks_create(){
    create_adc_read_task();
    create_decision_peltier();
}
//

