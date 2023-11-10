#include "tasks.h"

// Create FreeRTOS tasks
void create_tasks(){
    //create_adc_tasks();
    //create_ntc_tasks();
    create_control_system_task();
    create_dimmer_task();
}
//