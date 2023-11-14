#include "tasks.h"

// Create FreeRTOS tasks
void create_tasks(){
    //create_adc_tasks();
    //create_ntc_tasks();
    create_time_task();
    create_control_system_tasks();
    create_dimmer_task();
    create_power_tasks();
}
//