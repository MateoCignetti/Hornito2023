#include <power.h>
#include <dimmer.h>

void app_main(void){
    adc_init();
    setup_dimmer_isr();
    //create_sampling_timer();
    create_power_tasks();
    create_dimmer_task();
    //setup_power_isr();
}
