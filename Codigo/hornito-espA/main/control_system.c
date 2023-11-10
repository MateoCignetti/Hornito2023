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
    while (true) {
        temperature = get_ntc_temperature_c(get_adc_voltage_mv_multisampling(ADC_UNIT_1, ADC_CHANNEL_0));
        ESP_LOGI(TAG_CONTROL, "NTC1: %.2f °C", temperature);
        if (temperature < setPointTemperature) {
            set_dimmer_delay(1000);
            
        } else if (temperature > setPointTemperature) {
            set_dimmer_delay(9500);
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_CONTROL_SYSTEM_DELAY_MS));
    }
}
//