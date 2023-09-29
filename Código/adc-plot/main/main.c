#include "adc.h"

void app_main(void)
{
    adc1_init();
    create_adc_read_task();
}
