#include "dimmer.h"

// Defines
#define PIN_IN GPIO_NUM_34
#define PIN_OUT GPIO_NUM_33
#define DIMMER_SEMAPHORE_TIMEOUT_MS 11
#define PULSE_DELAY_US 10
//

// Handles
SemaphoreHandle_t xDimmerSemaphore_handle = NULL;
TaskHandle_t xTaskDimmer_handle = NULL;
static void dimmer_isr();
//

// Global variables
const static char* TAG_DIMMER = "DIMMER";
static int dimmer_delay_us = 9500; 
//

void setup_dimmer_isr(){
    gpio_set_direction(PIN_IN, GPIO_MODE_INPUT);
    gpio_set_intr_type(PIN_IN, GPIO_INTR_ANYEDGE);
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    gpio_isr_handler_add(PIN_IN, dimmer_isr, NULL);
    xDimmerSemaphore_handle = xSemaphoreCreateBinary();
}

static void IRAM_ATTR dimmer_isr(){
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xSemaphoreGiveFromISR(xDimmerSemaphore_handle, NULL);
    
    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

void create_dimmer_task(){
    xTaskCreatePinnedToCore(vTaskDimmer,
                            "Dimmer Task",
                            configMINIMAL_STACK_SIZE * 10,
                            NULL,
                            tskIDLE_PRIORITY + 2,
                            &xTaskDimmer_handle,
                            1);
}

void set_dimmer_delay(int new_dimmer_delay_us){
    dimmer_delay_us = new_dimmer_delay_us;
}

void vTaskDimmer(){
    gpio_set_direction(PIN_OUT, GPIO_MODE_OUTPUT);
    while(true){
        if(xSemaphoreTake(xDimmerSemaphore_handle, pdMS_TO_TICKS(DIMMER_SEMAPHORE_TIMEOUT_MS))){
            //ESP_LOGI(TAG_DIMMER, "Semaphore taken anashe");
            //ESP_LOGI(TAG_DIMMER, "Delay microseconds: %d", dimmer_delay_us);
            ets_delay_us(dimmer_delay_us);
            //vTaskDelay(pdMS_TO_TICKS(DELAY_DIMMER_MS));
            gpio_set_level(PIN_OUT, 1);
            ets_delay_us(PULSE_DELAY_US);
            gpio_set_level(PIN_OUT, 0);
        } else{
            ESP_LOGE(TAG_DIMMER, "Dimmer timeout");
        }
    }
}