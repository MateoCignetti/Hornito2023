#include "peltier.h"

//-------------------- Definiciones -------------

#define PIN_PELTIER_1 GPIO_NUM_14
#define PIN_PELTIER_2 GPIO_NUM_12
#define PIN_PUMP GPIO_NUM_13

#define ADC_UNIT ADC_UNIT_1
#define ADC_CHANNEL ADC_CHANNEL_6

#define SENSING_TIME_MS 10000

//-------------------- Prototipos -------------

static void vTaskDecision();
static void vTaskReadTemperatura();
static void vTaskSendData();
void create_peltier_tasks();
void delete_peltier_tasks();

//-------------------- Variables, constantes y punteros globales -------------

float temperature_c = 0.0;
const static char* TAG_PELTIER = "Peltier";
static SemaphoreHandle_t xTemperatureMutex = NULL;
static TaskHandle_t xTaskDecision_handle = NULL;
static TaskHandle_t xTaskReadTemperature_handle = NULL;
static TaskHandle_t xTaskSendData_handle = NULL;
QueueHandle_t xQueuePeltier = NULL; 
SemaphoreHandle_t xSemaphorePeltier = NULL;

void create_peltier_tasks(){
    xTaskCreatePinnedToCore(vTaskDecision,
                            "Decision Task", 
                            configMINIMAL_STACK_SIZE * 5,
                            NULL,
                            tskIDLE_PRIORITY + 1,
                            xTaskDecision_handle,
                            0);

    xTaskCreatePinnedToCore(vTaskReadTemperature,
                            "Read Task", 
                            configMINIMAL_STACK_SIZE * 5,
                            NULL,
                            tskIDLE_PRIORITY + 1,
                            xTaskReadTemperature_handle,
                            0);
    xTaskCreatePinnedToCore(vTaskSendData,
                            "Send Task", 
                            configMINIMAL_STACK_SIZE * 5,
                            NULL,
                            tskIDLE_PRIORITY + 1,
                            xTaskSendData_handle,
                            0);

    if(xTemperatureMutex == NULL){
        xTemperatureMutex = xSemaphoreCreateMutex();
    }
}

void delete_peltier_tasks(){
    vTaskDelete(xTaskDecision_handle);
    vTaskDelete(xTaskReadTemperature_handle);
    vTaskDelete(xTaskSendData_handle);
}

static void vTaskSendData(){

    if(xSemaphorePeltier == NULL){
        xSemaphorePeltier = xSemaphoreCreateBinary();
    }

    if(xQueuePeltier == NULL){
        xQueuePeltier = xQueueCreate(1, sizeof(float));
    }

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

static void vTaskDecision() {

    gpio_set_direction(PIN_PELTIER_1, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_PELTIER_2, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_PUMP, GPIO_MODE_OUTPUT);
    float temperatureDifference = 0.0;
    float setPointTemperature = 13.0;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (true) {
      
        if (xSemaphoreTake(mutexControlSystem, portMAX_DELAY)) {
            temperatureDifference = temperature_c - setPointTemperature;
            xSemaphoreGive(mutexControlSystem);
        }    

        if (temperatureDifference >= 2) {
            // Enciende las tres placas Peltier y la bomba
            gpio_set_level(PIN_PELTIER_1, 1);
            gpio_set_level(PIN_PELTIER_2, 1);
            gpio_set_level(PIN_PUMP, 1);
        } else if (temperatureDifference <= -2 ) {
            // Apaga las tres placas Peltier y la bomba
            gpio_set_level(PIN_PELTIER_1, 0);
            gpio_set_level(PIN_PELTIER_2, 0);
            gpio_set_level(PIN_PUMP, 0);
        } else {
            // Enciende una placa Peltier y la bomba
            gpio_set_level(PIN_PELTIER_1, 1);
            gpio_set_level(PIN_PELTIER_2, 0);
            gpio_set_level(PIN_PUMP, 1);
        }

        // Espera un tiempo antes de volver a leer la temperatura
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SENSING_TIME_MS));
    }
}