#include "control_system.h"

// Defines
#define TASK_CONTROL_SYSTEM_DELAY_MS 5000                               // Milisenconds to wait in the control system tasks
#define US_TO_STEPS 400                                                 // Conversion from microseconds to steps
#define CONTROL_MONITORING_TASK 0                                       // Defines and creates a task that monitors the control system tasks
#define CONTROL_MONITORING_TASK_DELAY_MS 2000                           // Defines the period of the control system monitoring task mentioned above
#define ADC_UNIT ADC_UNIT_1                                             // ADC unit to use
#define NTC1_CHANNEL ADC_CHANNEL_3                                      // ADC channel to use for NTC1

// Handles
static TaskHandle_t xTaskControlSystemGetTemperature_handle = NULL;     // Handle for the get temperature
static TaskHandle_t xTaskControlSystemSendTemperature_handle = NULL;    // Handle for the send temperature
static TaskHandle_t xTaskControlSystemDecision_handle = NULL;           // Handle for the decision
static TaskHandle_t xTaskControlSystemSendSteps_handle = NULL;          // Handle for the send steps
#if CONTROL_MONITORING_TASK
static TaskHandle_t xTaskControlSystemMonitor_handle = NULL;            // Handle for the monitor
#endif
static SemaphoreHandle_t mutexControlSystem = NULL;                     // Mutex to indicate that entry in the critical section
//

// Global variables
static float setPointTemperature = 37.0;                                // Value of set point temperature
static float temperature = 25.0;                                        // Initial value of temperature
int dimmer_delay_us = 9200;                                             // Initial value of dimmer delay in microseconds
QueueHandle_t xQueueControlSystem;                                      // Queue to send temperature to web
SemaphoreHandle_t xSemaphoreControlSystem;                              // Semaphore to indicate that temperature is ready to send
QueueHandle_t xQueueControlSystemToPower;                               // Queue to send steps to power
SemaphoreHandle_t xSemaphoreControlSystemToPower;                       // Semaphore to indicate that steps are ready to send
//

// ESP-LOG Tags
const static char* TAG_CONTROL = "CONTROL";                             // Tag for the control system
//

// Function prototypes
void create_control_system_tasks();
void create_control_system_semaphores_queues();
void delete_control_system_tasks();
static void vTaskControlSystemDecision();
static void vTaskControlSystemGetTemperature();
static void vTaskControlSystemSendTemperature();
static void vTaskControlSystemSendSteps();
#if CONTROL_MONITORING_TASK
static void vTaskControlSystemMonitor();
#endif
//

// Functions
// Create tasks and mutex for control system
void create_control_system_tasks(){
    if(mutexControlSystem == NULL){
        mutexControlSystem = xSemaphoreCreateMutex();
    }
    
    xTaskCreatePinnedToCore(vTaskControlSystemGetTemperature,
                            "Control System Get Temperature Task",
                            configMINIMAL_STACK_SIZE * 5,
                            NULL,
                            tskIDLE_PRIORITY + 2,
                            &xTaskControlSystemGetTemperature_handle,
                            0);

    xTaskCreatePinnedToCore(vTaskControlSystemSendTemperature,
                            "Control System Send Temperature Task",
                            configMINIMAL_STACK_SIZE * 5,
                            NULL,
                            tskIDLE_PRIORITY + 3,
                            &xTaskControlSystemSendTemperature_handle,
                            1);

    xTaskCreatePinnedToCore(vTaskControlSystemDecision,
                            "Control System Decision Task",
                            configMINIMAL_STACK_SIZE * 5,
                            NULL,
                            tskIDLE_PRIORITY + 1,
                            &xTaskControlSystemDecision_handle,
                            0);

    xTaskCreatePinnedToCore(vTaskControlSystemSendSteps,
                            "Control System Send Steps Task",
                            configMINIMAL_STACK_SIZE * 5,
                            NULL,
                            tskIDLE_PRIORITY + 4,
                            &xTaskControlSystemSendSteps_handle,
                            0); 
    #if CONTROL_MONITORING_TASK
    xTaskCreatePinnedToCore(vTaskControlSystemMonitor,
                            "Control System Monitor Task",
                            configMINIMAL_STACK_SIZE * 5,
                            NULL,
                            tskIDLE_PRIORITY + 1,
                            &xTaskControlSystemMonitor_handle,
                            1);
    #endif

}

void create_control_system_semaphores_queues(){
    xSemaphoreControlSystem = xSemaphoreCreateBinary();
    xSemaphoreControlSystemToPower = xSemaphoreCreateBinary();
    xQueueControlSystem = xQueueCreate(1, sizeof(float));
    xQueueControlSystemToPower = xQueueCreate(1, sizeof(float));
    ESP_LOGI(TAG_CONTROL, "Semaphores and queues created");
}

// Delete tasks for control system
void delete_control_system_tasks(){
    if(xTaskControlSystemGetTemperature_handle != NULL){
        vTaskDelete(xTaskControlSystemGetTemperature_handle);
        xTaskControlSystemGetTemperature_handle = NULL;
    }

    if(xTaskControlSystemSendTemperature_handle != NULL){
        vTaskDelete(xTaskControlSystemSendTemperature_handle);
        xTaskControlSystemSendTemperature_handle = NULL;
    }

    if(xTaskControlSystemDecision_handle != NULL){
        vTaskDelete(xTaskControlSystemDecision_handle);
        xTaskControlSystemDecision_handle = NULL;
    }

    if(xTaskControlSystemSendSteps_handle != NULL){
        vTaskDelete(xTaskControlSystemSendSteps_handle);
        xTaskControlSystemSendSteps_handle = NULL;
    }

    #if CONTROL_MONITORING_TASK
    if(xTaskControlSystemMonitor_handle != NULL){
        vTaskDelete(xTaskControlSystemMonitor_handle);
        xTaskControlSystemMonitor_handle = NULL;
    }
    #endif
}
//

// Task function
// Get temperature from NTC. Before get the temperature, the mutex is taken to indicate that the critical section is in use,
// and then the mutex is given to indicate that the critical section is free, when the temperature is got. 
static void vTaskControlSystemGetTemperature(){
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (true) {
        if (xSemaphoreTake(mutexControlSystem, portMAX_DELAY)) {
            temperature = get_ntc_temperature_c(get_adc_voltage_mv_multisampling(ADC_UNIT, NTC1_CHANNEL));
            xSemaphoreGive(mutexControlSystem);
        }
        ESP_LOGI(TAG_CONTROL, "NTC1: %.2f °C", temperature);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_CONTROL_SYSTEM_DELAY_MS));
    }
}

// Send temperature to web. First queue and semaphore are created that used to send temperature to web. In the while loop,
// the semaphore is taken to indicate the temperature is ready to send, and then the mutex is taken to indicate that
// critical section is in use. Finally, the temperature is sent to web and the mutex is given.
static void vTaskControlSystemSendTemperature(){
    
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

// Decision of the control system. First the mutex is taken, and then set temperature difference is calculated, between
// set point temperature and temperature. Finally, the dimmer delay is set according to the temperature difference and it sends
// its value to the dimmer task.
static void vTaskControlSystemDecision(){
    //int temperatureDifference = 0.0;
    int currentTemperatureDifference = 0;
    int previousTemperatureDifference = 0;
    int dimmer = 9200;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (true) {
        /*if (xSemaphoreTake(mutexControlSystem, portMAX_DELAY)) {
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
        */
        if (xSemaphoreTake(mutexControlSystem, portMAX_DELAY)) {
            currentTemperatureDifference = setPointTemperature - temperature;
            xSemaphoreGive(mutexControlSystem);
        }

        if (currentTemperatureDifference > 0) {
            if (currentTemperatureDifference == previousTemperatureDifference && dimmer_delay_us > 7200) {
                    dimmer -= 400;
            } else if (currentTemperatureDifference > previousTemperatureDifference && dimmer_delay_us > 7200) {
                    dimmer -= 400;
            } else if (currentTemperatureDifference < previousTemperatureDifference && dimmer_delay_us <= 9200){
                    dimmer += 400;
            }
            previousTemperatureDifference = currentTemperatureDifference; 
            dimmer_delay_us = dimmer;       
    
        } else {
            //dimmer_delay_us = 9200;
            disable_dimmer();
            ESP_LOGI(TAG_CONTROL, "Turn off dimmer");
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_CONTROL_SYSTEM_DELAY_MS));
            continue;
        }
        enable_dimmer();
        set_dimmer_delay(dimmer_delay_us);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_CONTROL_SYSTEM_DELAY_MS));     
        ESP_LOGI(TAG_CONTROL, "Delay microseconds: %d", dimmer_delay_us); 
    }
}

// Send steps to power. Queue and Semaphore are created at the same form as vTaskControlSystemSendTemperature()
// In the while loop, It sets the steps according to the dimmer delay and sends its value to power task.
static void vTaskControlSystemSendSteps(){
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

// Monitor the tasks. Debugging function to monitor the tasks memory.
#if CONTROL_MONITORING_TASK
static void vTaskControlSystemMonitor(){
    while (true) {
        ESP_LOGW(TAG_CONTROL, "Control task get temperature: %u bytes", uxTaskGetStackHighWaterMark(xTaskControlSystemGetTemperature_handle));
        ESP_LOGW(TAG_CONTROL, "Control task send temperature: %u bytes", uxTaskGetStackHighWaterMark(xTaskControlSystemSendTemperature_handle));
        ESP_LOGW(TAG_CONTROL, "Control task decision: %u bytes", uxTaskGetStackHighWaterMark(xTaskControlSystemDecision_handle));
        ESP_LOGW(TAG_CONTROL, "Control task send steps: %u bytes", uxTaskGetStackHighWaterMark(xTaskControlSystemSendSteps_handle));
        vTaskDelay(pdMS_TO_TICKS(CONTROL_MONITORING_TASK_DELAY_MS));
    }
}
#endif
//