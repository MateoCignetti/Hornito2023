#include "peltier.h"

// Define

#define PIN_PELTIER_1 GPIO_NUM_14   //Pin where peltier 1 is connected
#define PIN_PELTIER_2 GPIO_NUM_12   //Pin where peltier 2 is connected
#define PIN_PUMP GPIO_NUM_13        //Pin where pump is connected

#define ADC_UNIT ADC_UNIT_1         //ADC unit used 
#define ADC_CHANNEL ADC_CHANNEL_6   //ADC channel used

#define SENSING_TIME_MS 10000       // Temperature sensing time

#define PELTIER_MONITORING_TASK 0   // Enables the peltier task stack size monitoring
#define TASK_MONITOR_DELAY_MS 2000  // Time interval for the monitoring task
//

// Function prototypes
static void vTaskDecision();
static void vTaskReadTemperature();
static void vTaskSendData();
#if PELTIER_MONITORING_TASK
static void vTaskPeltierMonitor();
#endif 
void create_peltier_tasks();
void delete_peltier_tasks();
void create_peltier_semaphores_queues();
//

// Global variables
float temperature_c = 0.0;                                  // Global variable for temperature
const static char* TAG_PELTIER = "Peltier";                 // Tag for log messages
static SemaphoreHandle_t xTemperatureMutex = NULL;          // Mutex to protect access to the variable temperature_c
static TaskHandle_t xTaskReadTemperature_handle = NULL;     // Handle from the read temperature task 
static TaskHandle_t xTaskSendData_handle = NULL;            // Handle from the send data task
static TaskHandle_t xTaskDecision_handle = NULL;            // Handle from the desicion task

QueueHandle_t xQueuePeltier = NULL;                         //Queue to send the temperature value
SemaphoreHandle_t xSemaphorePeltier = NULL;                 //Semaphore to indicate that temperature value must be sent
#if PELTIER_MONITORING_TASK
TaskHandle_t xTaskPeltierMonitor_handle = NULL;             // Handle from the monitor task
#endif
//

// Function to create tasks
void create_peltier_tasks(){

    // Create a mutex to protect the variable temperature_c
    if(xTemperatureMutex == NULL){
        xTemperatureMutex = xSemaphoreCreateMutex();
    }
    
    xTaskCreatePinnedToCore(vTaskDecision,
                            "Decision Task", 
                            configMINIMAL_STACK_SIZE * 5,
                            NULL,
                            tskIDLE_PRIORITY + 1,
                            &xTaskDecision_handle,
                            0);

    xTaskCreatePinnedToCore(vTaskReadTemperature,
                            "Read Task", 
                            configMINIMAL_STACK_SIZE * 5,
                            NULL,
                            tskIDLE_PRIORITY + 1,
                            &xTaskReadTemperature_handle,
                            0);
    xTaskCreatePinnedToCore(vTaskSendData,
                            "Send Task", 
                            configMINIMAL_STACK_SIZE * 5,
                            NULL,
                            tskIDLE_PRIORITY + 1,
                            &xTaskSendData_handle,
                            0);
    #if PELTIER_MONITORING_TASK
    xTaskCreatePinnedToCore(vTaskPeltierMonitor,
                            "Monitor Task", 
                            configMINIMAL_STACK_SIZE * 5,
                            NULL,
                            tskIDLE_PRIORITY + 1,
                            &xTaskPeltierMonitor_handle,
                            0);
    #endif
}

// Function to delete tasks
void delete_peltier_tasks(){
    vTaskDelete(xTaskDecision_handle);
    vTaskDelete(xTaskReadTemperature_handle);
    vTaskDelete(xTaskSendData_handle);
    #if PELTIER_MONITORING_TASK
    vTaskDelete(xTaskPeltierMonitor_handle);
    #endif
}
//

// Function to create semaphore and queue
void create_peltier_semaphores_queues(){
    xSemaphorePeltier = xSemaphoreCreateBinary();
    xQueuePeltier = xQueueCreate(1, sizeof(float));
}
// 

// This task is responsible for sending temperature data to a queue. Periodically,
// it waits for the semaphore signal, and upon acquiring it, obtains the mutex to
// access the temperature variable. Subsequently, it sends the temperature value to
// the queue and logs messages to indicate the success or failure of the transmission.
static void vTaskSendData(){
    while (true){
        if (xSemaphoreTake(xSemaphorePeltier, portMAX_DELAY)){
            if (xSemaphoreTake(xTemperatureMutex, portMAX_DELAY)){
                if (xQueueSend(xQueuePeltier, &temperature_c, portMAX_DELAY) != pdPASS){
                    ESP_LOGE(TAG_PELTIER, "Error sending temperature value to queue\n");
                }else{
                    ESP_LOGI(TAG_PELTIER, "Sent %.2f °C value to temperature queue", temperature_c);
                }
                xSemaphoreGive(xTemperatureMutex);
            }
        }
    }
}
//

// This task is responsible for reading the temperature from an NTC sensor. It periodically
// reads the temperature using the NTC sensor and the analog-to-digital converter (ADC). It
// acquires a mutex before accessing the global temperature variable. The temperature value
// is updated based on the NTC sensor reading, and the mutex is released after the update.
// It waits for a specified time before reading the temperature again.
static void vTaskReadTemperature(){
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (true){
        if(xSemaphoreTake(xTemperatureMutex, portMAX_DELAY)){
            temperature_c = get_ntc_temperature_c(get_adc_voltage_mv_multisampling(ADC_UNIT, ADC_CHANNEL));
            xSemaphoreGive(xTemperatureMutex);
        }
        ESP_LOGI(TAG_PELTIER, "NTC Peltier: %.2f °C ", temperature_c);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SENSING_TIME_MS));
    }
}
//

// Task assigned to make control decisions based on temperature. Configures the 
// pins as outputs for the Peltier plates and the pump. Calculates the difference 
// between the current temperature and the desired temperature. Based on the 
// temperature difference, it controls the turning on and off of the plates and 
// the pump. If the difference is greater than two, it turns on all the plates and 
// the pump. If it is less than minus two, it turns everything off. In an 
// intermediate case, it keeps one plate and the pump on.
static void vTaskDecision() {

    gpio_set_direction(PIN_PELTIER_1, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_PELTIER_2, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_PUMP, GPIO_MODE_OUTPUT);
    float temperatureDifference = 0.0;
    float setPointTemperature = 13.0;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (true) {
      
        if (xSemaphoreTake(xTemperatureMutex, portMAX_DELAY)) {
            temperatureDifference = temperature_c - setPointTemperature;
            xSemaphoreGive(xTemperatureMutex);
        }    

        if (temperatureDifference >= 2) {
            gpio_set_level(PIN_PELTIER_1, 1);
            gpio_set_level(PIN_PELTIER_2, 1);
            gpio_set_level(PIN_PUMP, 1);
        } else if (temperatureDifference <= -2 ) {
            gpio_set_level(PIN_PELTIER_1, 0);
            gpio_set_level(PIN_PELTIER_2, 0);
            gpio_set_level(PIN_PUMP, 0);
        } else {
            gpio_set_level(PIN_PELTIER_1, 1);
            gpio_set_level(PIN_PELTIER_2, 0);
            gpio_set_level(PIN_PUMP, 1);
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SENSING_TIME_MS));
    }
}
//

// Task to monitor stack usage
#if PELTIER_MONITORING_TASK
void vTaskPeltierMonitor(){
    while(true){
        
        ESP_LOGW(TAG_PELTIER, "Task enviar temp: %u bytes", uxTaskGetStackHighWaterMark(xTaskSendData_handle));
        ESP_LOGW(TAG_PELTIER, "Task leer temp: %u bytes", uxTaskGetStackHighWaterMark(xTaskReadTemperature_handle));
        ESP_LOGW(TAG_PELTIER, "Task desicion: %u bytes", uxTaskGetStackHighWaterMark(xTaskDecision_handle));
        
        vTaskDelay(pdMS_TO_TICKS(TASK_MONITOR_DELAY_MS));
    }
}
#endif
//
