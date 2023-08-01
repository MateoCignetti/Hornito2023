#include "dimmer.h"

// Defines
#define PIN_IN GPIO_NUM_4
#define PIN_OUT GPIO_NUM_2
#define DELAY_DIMMER_MS 2
#define DIMMER_SEMAPHORE_TIMEOUT_MS 6
//

// Handles
SemaphoreHandle_t xDimmerSemaphore = NULL;
TaskHandle_t xTaskDimmer_handle = NULL;
void isr_handler(void* arg);
//

// ESP-LOG Tags
const static char* TAG_DIMMER = "DIMMER";
//

void setup_dimmer_isr(){
    gpio_set_direction(PIN_IN, GPIO_MODE_INPUT);
    gpio_set_intr_type(PIN_IN, GPIO_INTR_ANYEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_IN, isr_handler, (void*) PIN_IN);
}

void isr_handler(void* arg){
    uint32_t gpio_num = (uint32_t) arg;
    if(gpio_num == PIN_IN){
        xSemaphoreGiveFromISR(xDimmerSemaphore, pdTRUE);
    }
}

void create_dimmer_task(){
    xTaskCreate(vTaskDimmer,
                "Dimmer Task",
                configMINIMAL_STACK_SIZE * 5,
                NULL,
                tskIDLE_PRIORITY + 2,
                &xTaskDimmer_handle);
}

void vTaskDimmer(){
    gpio_set_direction(PIN_OUT, GPIO_MODE_OUTPUT);

    while(true){
        if(xSemaphoreTake(xDimmerSemaphore, pdMS_TO_TICKS(DIMMER_SEMAPHORE_TIMEOUT_MS))){
            vTaskDelay(pdMS_TO_TICKS(DELAY_DIMMER_MS));
            gpio_set_level(PIN_OUT, 1);
            ets_delay_us(5);
            gpio_set_level(PIN_OUT, 0);
        } else{
            ESP_LOGE(TAG_DIMMER, "Dimmer timeout");
        }
    }
}