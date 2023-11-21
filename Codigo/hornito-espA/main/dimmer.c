#include "dimmer.h"

// Defines
#define PIN_IN GPIO_NUM_26                          // Defines the zero-crossing circuit input pi
#define PIN_OUT GPIO_NUM_27                         // Defines the TRIAC driver output pin
#define DIMMER_SEMAPHORE_TIMEOUT_MS 12              // Defines the timeout for the dimmer semaphore in milliseconds
#define PULSE_DELAY_US 10                           // Defines the pulse width of the output signal in microseconds

#define DIMMER_MONITORING_TASK 0                    // Enables the dimmer task stack size monitoring
#define MONITORING_DELAY_MS 2000                    // Defines the delay between monitoring in milliseconds
//

// Handles
SemaphoreHandle_t xDimmerSemaphore_handle = NULL;   // Defines the dimmer semaphore handle
TaskHandle_t xTaskDimmer_handle = NULL;             // Defines the dimmer task handle
#if DIMMER_MONITORING_TASK
TaskHandle_t xTaskDimmerMonitoring_handle = NULL;   // Defines the dimmer monitoring task handle
#endif
//

// Global variables
const static char* TAG_DIMMER = "DIMMER";           // Defines the tag for logging
static int dimmer_delay_us = 9500;                  // Variable for the dimmer delay in microseconds
bool dimmer_enabled = true;                         // Flag to indicate if the dimmer is enabled
//

// Function prototypes
static void vTaskDimmer();
#if DIMMER_MONITORING_TASK
static void vTaskDimmerMonitoring();
#endif
void setup_dimmer_isr();
void create_dimmer_task();
void delete_dimmer_task();
void set_dimmer_delay(int new_dimmer_delay_us);
void enable_dimmer();
void disable_dimmer();
//

// Functions

// Dimmer ISR. Releases the dimmer semaphore when an edge is detected.
static void IRAM_ATTR dimmer_isr(){
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xSemaphoreGiveFromISR(xDimmerSemaphore_handle, NULL);
    
    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

// Function to setup the dimmer ISR and semaphore. Should be called from main.c
void setup_dimmer_isr(){
    gpio_set_direction(PIN_IN, GPIO_MODE_INPUT);
    gpio_set_intr_type(PIN_IN, GPIO_INTR_ANYEDGE); // Interrupt on any edge
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    gpio_isr_handler_add(PIN_IN, dimmer_isr, NULL);
    xDimmerSemaphore_handle = xSemaphoreCreateBinary();
}

// Creates the dimmer task
void create_dimmer_task(){
    xTaskCreatePinnedToCore(vTaskDimmer,
                            "Dimmer Task",
                            configMINIMAL_STACK_SIZE * 5,
                            NULL,
                            tskIDLE_PRIORITY + 6,
                            &xTaskDimmer_handle,
                            1);
    #if DIMMER_MONITORING_TASK
    xTaskCreatePinnedToCore(vTaskDimmerMonitoring,
                            "Dimmer Monitoring Task",
                            configMINIMAL_STACK_SIZE * 5,
                            NULL,
                            tskIDLE_PRIORITY + 1,
                            &xTaskDimmerMonitoring_handle,
                            1);
    #endif
}

// Deletes the dimmer task
void delete_dimmer_task(){
    if(xTaskDimmer_handle != NULL){
        vTaskDelete(xTaskDimmer_handle);
        xTaskDimmer_handle = NULL;
    }

    #if DIMMER_MONITORING_TASK
    if(xTaskDimmerMonitoring_handle != NULL){
        vTaskDelete(xTaskDimmerMonitoring_handle);
        xTaskDimmerMonitoring_handle = NULL;
    }
    #endif
}

// Sets the dimmer delay in microseconds
void set_dimmer_delay(int new_dimmer_delay_us){
    dimmer_delay_us = new_dimmer_delay_us;
}

// Enables the dimmer
void enable_dimmer(){
    dimmer_enabled = true;
}

// Disables the dimmer
void disable_dimmer(){
    dimmer_enabled = false;
}

// Dimmer task. It waits for the dimmer semaphore to be released and then it outputs a pulse after
// a certain delay. The delay is set by the control system task.
static void vTaskDimmer(){
    gpio_set_direction(PIN_OUT, GPIO_MODE_OUTPUT);
    while(true){
        if(xSemaphoreTake(xDimmerSemaphore_handle, pdMS_TO_TICKS(DIMMER_SEMAPHORE_TIMEOUT_MS))){
            if(dimmer_enabled){
                ets_delay_us(dimmer_delay_us);
                gpio_set_level(PIN_OUT, 1);
                ets_delay_us(PULSE_DELAY_US);
                gpio_set_level(PIN_OUT, 0);
            } else{
                gpio_set_level(PIN_OUT, 0);
            }
            //ESP_LOGI(TAG_DIMMER, "Semaphore taken");
            //ESP_LOGI(TAG_DIMMER, "Delay microseconds: %d", dimmer_delay_us);
            //vTaskDelay(pdMS_TO_TICKS(DELAY_DIMMER_MS));
        } else{
            ESP_LOGE(TAG_DIMMER, "Dimmer timeout");     // Could be modified so that it doesn't flood the output. Maybe a counter?
            //vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

#if DIMMER_MONITORING_TASK
// Dimmer monitoring task. It monitors the dimmer task stack size and prints it to the output.
static void vTaskDimmerMonitoring(){
    while(true){
        ESP_LOGW(TAG_DIMMER, "Dimmer task stack size: %d", uxTaskGetStackHighWaterMark(xTaskDimmer_handle));
        vTaskDelay(pdMS_TO_TICKS(MONITORING_DELAY_MS));
    }
}
#endif