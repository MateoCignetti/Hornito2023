// Includes
#include "adc.h"
#include "peltier.h"
#include "ntc.h"
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
    adc_init();
    tasks_create();
}
//

// Functions

//

// FreeRTOS Task create
void tasks_create(){
    //create_adc_read_task();
    create_peltier_tasks();
    create_ntc_tasks();
}
//

