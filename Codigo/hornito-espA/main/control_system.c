#include "control_system.h"

// Defines
#define TASK_CONTROL_SYSTEM_DELAY_MS 2000

// Handles
TaskHandle_t xTaskControlSystem_handle = NULL;
//

// ESP-LOG Tags
const static char* TAG_CONTROL = "CONTROL";
//

// Global variables
static float setPointTemperature = 60.0;
//

// Function prototypes
static void vTaskControlSystem();
//

// Task
void create_control_system_task(){
    xTaskCreatePinnedToCore(vTaskControlSystem,
                            "Control System Task",
                            configMINIMAL_STACK_SIZE * 10,
                            NULL,
                            tskIDLE_PRIORITY + 1,
                            &xTaskControlSystem_handle,
                            0);
}
//

// Task function
static void vTaskControlSystem(){
    float temperature = 25.0;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    int dimmer_delay_us = 9500;
    while (true) {
        temperature = get_ntc_temperature_c(get_adc_voltage_mv_multisampling(ADC_UNIT_1, ADC_CHANNEL_0));
        ESP_LOGI(TAG_CONTROL, "NTC1: %.2f °C", temperature);
        if (temperature < setPointTemperature) {
            dimmer_delay_us = 3000;  
        } else if (temperature > setPointTemperature) {
            dimmer_delay_us = 9500;
        }
        set_dimmer_delay(dimmer_delay_us);
        ESP_LOGI(TAG_CONTROL, "Delay microseconds: %d", dimmer_delay_us);

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_CONTROL_SYSTEM_DELAY_MS));
    }
}
//