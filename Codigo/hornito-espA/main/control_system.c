#include "control_system.h"

// Defines
#define TASK_CONTROL_SYSTEM_DELAY_MS 2000
#define US_TO_STEPS 400
#define TASK_MONITOR_DELAY_MS 2000

// Handles
static TaskHandle_t xTaskControlSystemGetTemperature_handle = NULL;
static TaskHandle_t xTaskControlSystemSendTemperature_handle = NULL;
static TaskHandle_t xTaskControlSystemDecision_handle = NULL;
static TaskHandle_t xTaskControlSystemSendSteps_handle = NULL;
static TaskHandle_t xTaskControlSystemMonitor_handle = NULL;
static SemaphoreHandle_t mutexControlSystem = NULL;
//

// ESP-LOG Tags
const static char* TAG_CONTROL = "CONTROL";
//

// Global variables
static float setPointTemperature = 60.0;
static float temperature = 25.0;
int dimmer_delay_us = 9600;
QueueHandle_t xQueueControlSystem;
SemaphoreHandle_t xSemaphoreControlSystem;
QueueHandle_t xQueueControlSystemToPower;
SemaphoreHandle_t xSemaphoreControlSystemToPower;
//

// Function prototypes
static void create_control_system_mutex();
static void vTaskControlSystemDecision();
static void vTaskControlSystemGetTemperature();
static void vTaskControlSystemSendTemperature();
static void vTaskControlSystemSendSteps();
static void vTaskControlSystemMonitor();
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
                            1);

    xTaskCreatePinnedToCore(vTaskControlSystemDecision,
                            "Control System Decision Task",
                            configMINIMAL_STACK_SIZE * 10,
                            NULL,
                            tskIDLE_PRIORITY + 1,
                            &xTaskControlSystemDecision_handle,
                            0);

    xTaskCreatePinnedToCore(vTaskControlSystemSendSteps,
                            "Control System Send Steps Task",
                            configMINIMAL_STACK_SIZE * 10,
                            NULL,
                            tskIDLE_PRIORITY + 4,
                            &xTaskControlSystemSendSteps_handle,
                            0); 

    xTaskCreatePinnedToCore(vTaskControlSystemMonitor,
                            "Control System Monitor Task",
                            configMINIMAL_STACK_SIZE * 5,
                            NULL,
                            tskIDLE_PRIORITY + 1,
                            &xTaskControlSystemMonitor_handle,
                            1);

    create_control_system_mutex();
}

void delete_control_system_tasks(){
    vTaskDelete(xTaskControlSystemGetTemperature_handle);
    vTaskDelete(xTaskControlSystemSendTemperature_handle);
    vTaskDelete(xTaskControlSystemDecision_handle);
    vTaskDelete(xTaskControlSystemSendSteps_handle);
    vTaskDelete(xTaskControlSystemMonitor_handle);
}
//

// Task function
static void vTaskControlSystemGetTemperature(){
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (true) {
        if (xSemaphoreTake(mutexControlSystem, portMAX_DELAY)) {
            temperature = get_ntc_temperature_c(get_adc_voltage_mv_multisampling(ADC_UNIT_1, ADC_CHANNEL_3));
            xSemaphoreGive(mutexControlSystem);
        }
        ESP_LOGI(TAG_CONTROL, "NTC1: %.2f °C", temperature);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_CONTROL_SYSTEM_DELAY_MS));
    }
}

static void vTaskControlSystemSendTemperature(){
    if(xQueueControlSystem == NULL){
        xQueueControlSystem = xQueueCreate(1, sizeof(float));
    }
    
    if(xSemaphoreControlSystem == NULL){
        xSemaphoreControlSystem = xSemaphoreCreateBinary();
    }
    
    while (true) {
        if (xSemaphoreTake(xSemaphoreControlSystem, portMAX_DELAY)) {
            if (xSemaphoreTake(mutexControlSystem, portMAX_DELAY)) {
                if (xQueueSend(xQueueControlSystem, &temperature, portMAX_DELAY) != pdPASS) {
                    ESP_LOGE(TAG_CONTROL, "Error sending temperature to web");
                }
                xSemaphoreGive(mutexControlSystem);
            }
        }
    }
}

static void vTaskControlSystemDecision(){
    int temperatureDifference = 0.0;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (true) {
        if (xSemaphoreTake(mutexControlSystem, portMAX_DELAY)) {
            temperatureDifference = setPointTemperature - temperature;
            xSemaphoreGive(mutexControlSystem);
        }

        if (temperatureDifference <= 60 && temperatureDifference > 50) {
            dimmer_delay_us = 7200;  
        } else if (temperatureDifference <= 50 && temperatureDifference > 40) {
            dimmer_delay_us = 7600;
        } else if (temperatureDifference <= 40 && temperatureDifference> 30) {
            dimmer_delay_us = 8000;
        } else if (temperatureDifference <= 30 && temperatureDifference > 20) {
            dimmer_delay_us = 8400;
        } else if (temperatureDifference <= 20 && temperatureDifference > 10) {
             dimmer_delay_us = 8800;
        } else if (temperatureDifference <= 10 && temperatureDifference > 0) {
             dimmer_delay_us = 9200;
        } else {
            disable_dimmer();
            continue;
        }
        enable_dimmer();
        set_dimmer_delay(dimmer_delay_us);
        ESP_LOGI(TAG_CONTROL, "Delay microseconds: %d", dimmer_delay_us);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_CONTROL_SYSTEM_DELAY_MS));
    }
}

static void vTaskControlSystemSendSteps(){
    if(xQueueControlSystemToPower == NULL){
        xQueueControlSystemToPower = xQueueCreate(1, sizeof(float));
    }

    if(xSemaphoreControlSystemToPower == NULL){
        xSemaphoreControlSystemToPower = xSemaphoreCreateBinary();
    }

    int steps = 0;

    while (true) {
        if (xSemaphoreTake(mutexControlSystem, portMAX_DELAY)) {
            steps = dimmer_delay_us/US_TO_STEPS;
            xSemaphoreGive(mutexControlSystem);
        }
        if (xSemaphoreTake(xSemaphoreControlSystemToPower, portMAX_DELAY)) {
                if (xQueueSend(xQueueControlSystemToPower, &steps, portMAX_DELAY) != pdTRUE) {
                    ESP_LOGE(TAG_CONTROL, "Error sending steps to power");
                }
        }
    }
}

static void vTaskControlSystemMonitor(){
    while (true) {
        if(xTaskControlSystemGetTemperature_handle != NULL){ 
            UBaseType_t taskGetTemperature_memory = uxTaskGetStackHighWaterMark(xTaskControlSystemGetTemperature_handle);
            ESP_LOGW(TAG_CONTROL, "Task leer temp: %u bytes", taskGetTemperature_memory);
            if(taskGetTemperature_memory == 0){
                xTaskControlSystemGetTemperature_handle = NULL;
            }
        }
        if(xTaskControlSystemSendTemperature_handle != NULL){ 
            UBaseType_t taskSendTemperature_memory = uxTaskGetStackHighWaterMark(xTaskControlSystemSendTemperature_handle);
            ESP_LOGW(TAG_CONTROL, "Task leer temp: %u bytes", taskSendTemperature_memory);
            if(taskSendTemperature_memory == 0){
                xTaskControlSystemSendTemperature_handle = NULL;
            }
        }
        if(xTaskControlSystemDecision_handle != NULL){ 
            UBaseType_t taskDecision_memory = uxTaskGetStackHighWaterMark(xTaskControlSystemDecision_handle);
            ESP_LOGW(TAG_CONTROL, "Task leer temp: %u bytes", taskDecision_memory);
            if(taskDecision_memory == 0){
                xTaskControlSystemDecision_handle = NULL;
            }
        }
        if(xTaskControlSystemSendSteps_handle != NULL){ 
            UBaseType_t taskSendSteps_memory = uxTaskGetStackHighWaterMark(xTaskControlSystemSendSteps_handle);
            ESP_LOGW(TAG_CONTROL, "Task leer temp: %u bytes", taskSendSteps_memory);
            if(taskSendSteps_memory == 0){
                xTaskControlSystemSendSteps_handle = NULL;
            }
        }        
        vTaskDelay(pdMS_TO_TICKS(TASK_MONITOR_DELAY_MS));
    }
}
//

// Functions
void create_control_system_mutex(){
    if (mutexControlSystem == NULL) {
        mutexControlSystem = xSemaphoreCreateMutex();
    }
}
//
