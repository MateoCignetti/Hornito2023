#include <power.h>

void app_main(void){
    adc_init();
    //create_sampling_timer();
    create_power_tasks();
    setup_power_isr();
}
