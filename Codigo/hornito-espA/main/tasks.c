#include "tasks.h"

void initialize_tasks(){
    create_control_system_semaphores_queues();
    create_peltier_semaphores_queues();
    create_power_semaphores_queues();
}

// Create FreeRTOS tasks
void create_tasks(){
    //create_adc_tasks();
    //create_ntc_tasks();
    //create_time_task();
    create_control_system_tasks();
    create_dimmer_task();
    create_peltier_tasks();
    create_power_tasks();
}

void delete_tasks(){
    //delete_adc_tasks();
    //delete_ntc_tasks();
    //delete_time_task();
    delete_control_system_tasks();
    delete_dimmer_task();
    delete_peltier_tasks();
    delete_power_tasks();
}
//