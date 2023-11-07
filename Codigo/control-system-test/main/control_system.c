#include "control_system.h"

// Handles
TaskHandle_t xTaskControlSystem_handle = NULL;
//

// Local variables
static float setPointTemperature = 60;
//

// Function prototypes
void vTaskControlSystem(void);
//

// Task
void create_control_system_task(){
    xTaskCreatePinnedToCore(vTaskControlSystem,
                            "Control System Task",
                            configMINIMAL_STACK_SIZE * 10,
                            NULL,
                            tskIDLE_PRIORITY + 2,
                            &xTaskControlSystem_handle,
                            1);
}
//

// Task function
void vTaskControlSystem(void){
    float temperature = 0;
    while (true) {
        temperature = get_ntc_temperature_c(get_adc_voltage_mv_multisampling(ADC_UNIT_1, ADC_CHANNEL_0));
        if (temperature < setPointTemperature) {
            delayDimmerMs = 1;
        } else if (temperature > setPointTemperature) {
            delayDimmerMs = 0;
        }
    }
}
//