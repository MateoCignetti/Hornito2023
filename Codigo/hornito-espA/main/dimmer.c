#include "dimmer.h"

// Defines
#define PIN_IN GPIO_NUM_32
#define PIN_OUT GPIO_NUM_33
#define DELAY_DIMMER_MS 7
#define DIMMER_SEMAPHORE_TIMEOUT_MS 11
//

// Handles
SemaphoreHandle_t xDimmerSemaphore_handle = NULL;
TaskHandle_t xTaskDimmer_handle = NULL;
void isr_handler(void* arg);
//

// ESP-LOG Tags
const static char* TAG_DIMMER = "DIMMER";
//

void setup_dimmer_isr(){
    gpio_set_direction(PIN_IN, GPIO_MODE_INPUT);
    gpio_set_intr_type(PIN_IN, GPIO_INTR_ANYEDGE);
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    gpio_isr_handler_add(PIN_IN, isr_handler, (void*) PIN_IN);
    xDimmerSemaphore_handle = xSemaphoreCreateBinary();
}

void isr_handler(void* arg){
    uint32_t gpio_num = (uint32_t) arg;
    if(gpio_num == PIN_IN){
        xSemaphoreGiveFromISR(xDimmerSemaphore_handle, NULL);
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

void vTaskDimmer(){
    gpio_set_direction(PIN_OUT, GPIO_MODE_OUTPUT);
    while(true){
        if(xSemaphoreTake(xDimmerSemaphore_handle, pdMS_TO_TICKS(DIMMER_SEMAPHORE_TIMEOUT_MS))){
            //ESP_LOGI(TAG_DIMMER, "Semaphore taken anashe");
            ets_delay_us(DELAY_DIMMER_MS*1000);
            //vTaskDelay(pdMS_TO_TICKS(DELAY_DIMMER_MS));
            gpio_set_level(PIN_OUT, 1);
            ets_delay_us(10);
            gpio_set_level(PIN_OUT, 0);
        } else{
            ESP_LOGE(TAG_DIMMER, "Dimmer timeout");
        }
    }
}