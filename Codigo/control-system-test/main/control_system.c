#include "control_system.h"

// Defines
#define TASK_CONTROL_SYSTEM_DELAY_MS 2000

// Handles
static TaskHandle_t xTaskControlSystemGetTemperature_handle = NULL;
static TaskHandle_t xTaskControlSystemSendTemperature_handle = NULL;
static TaskHandle_t xTaskControlSystemDecision_handle = NULL;
static SemaphoreHandle_t mutexControlSystem = NULL;
//

// ESP-LOG Tags
const static char* TAG_CONTROL = "CONTROL";
//

// Global variables
static float setPointTemperature = 60.0;
static float temperature = 25.0;
QueueHandle_t xQueueControlSystem;
SemaphoreHandle_t xSemaphoreControlSystem;
//

// Function prototypes
static void create_control_system_mutex();
static void vTaskControlSystemDecision();
static void vTaskControlSystemGetTemperature();
static void vTaskControlSystemSendTemperature();
//

// Task
void create_control_system_tasks(){
    xTaskCreatePinnedToCore(vTaskControlSystemGetTemperature,
                            "Control System Get Temperature Task",
                            configMINIMAL_STACK_SIZE * 10,
                            NULL,
                            tskIDLE_PRIORITY + 2,
                            &xTaskControlSystemGetTemperature_handle,
                            1);
    xTaskCreatePinnedToCore(vTaskControlSystemSendTemperature,
                            "Control System Send Temperature Task",
                            configMINIMAL_STACK_SIZE * 10,
                            NULL,
                            tskIDLE_PRIORITY + 3,
                            &xTaskControlSystemSendTemperature_handle,
                            0);
    xTaskCreatePinnedToCore(vTaskControlSystemDecision,
                            "Control System Decision Task",
                            configMINIMAL_STACK_SIZE * 10,
                            NULL,
                            tskIDLE_PRIORITY + 1,
                            &xTaskControlSystemDecision_handle,
                            1);
    create_control_system_mutex();
}
//

// Task function
static void vTaskControlSystemGetTemperature(){
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (true) {
        if (xSemaphoreTake(mutexControlSystem, NULL)) {
            temperature = get_ntc_temperature_c(get_adc_voltage_mv_multisampling(ADC_UNIT_1, ADC_CHANNEL_0));
            xSemaphoreGive(mutexControlSystem);
        }
        ESP_LOGI(TAG_CONTROL, "NTC1: %.2f °C", temperature);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_CONTROL_SYSTEM_DELAY_MS));
    }
}

static void vTaskControlSystemSendTemperature(){
    xQueueControlSystem = xQueueCreate(1, sizeof(float));
    xSemaphoreControlSystem = xSemaphoreCreateBinary();
    while (true) {
        if (xSemaphoreTake(xSemaphoreControlSystem, portMAX_DELAY)) {
            xSemaphoreTake(mutexControlSystem, portMAX_DELAY);
            if (xQueueSend(xQueueControlSystem, &temperature, portMAX_DELAY) != pdTRUE) {
                xSemaphoreGive(mutexControlSystem);
                ESP_LOGE(TAG_CONTROL, "Error sending temperature to web");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(TASK_CONTROL_SYSTEM_DELAY_MS));
    }
}

static void vTaskControlSystemDecision(){
    int temperatureDifference = 0.0;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    int dimmer_delay_us = 9500;
    while (true) {
        if (xSemaphoreTake(mutexControlSystem, NULL)) {
            temperatureDifference = setPointTemperature - temperature;
            xSemaphoreGive(mutexControlSystem);
        }
            if (temperatureDifference < 60 && temperatureDifference > 50) {
                dimmer_delay_us = 800;  
            } else if (temperatureDifference < 50 && temperatureDifference > 40) {
                dimmer_delay_us = 2400;
            } else if (temperatureDifference < 40 && temperatureDifference> 30) {
                dimmer_delay_us = 4000;
            } else if (temperatureDifference < 30 && temperatureDifference > 20) {
                dimmer_delay_us = 6000;
            } else if (temperatureDifference < 20 && temperatureDifference > 10) {
                dimmer_delay_us = 8000;
            } else if (temperatureDifference < 10 && temperatureDifference > 0) {
                dimmer_delay_us = 9000;
            } else {
                dimmer_delay_us = 9200;
            }
        set_dimmer_delay(dimmer_delay_us);
        ESP_LOGI(TAG_CONTROL, "Delay microseconds: %d", dimmer_delay_us);
    }
}
//

// Functions
void create_control_system_mutex(){
    mutexControlSystem = xSemaphoreCreateMutex();
}
//