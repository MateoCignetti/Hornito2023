// Includes
#include "tasks.h"
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
//

// Main
void app_main(void){
    setup_time();
    adc_init();
    setup_dimmer_isr();
    setup_wifi();
    initialize_tasks();
    //create_tasks();
    start_webserver();
}
//

// Functions

//

//

