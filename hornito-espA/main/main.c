// Includes
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//

// Defines
#define ADC1_DESIRED_BITWIDTH ADC_BITWIDTH_12
#define MULTISAMPLE_SIZE 64
#define MULTISAMPLE_DELAY_MS 1
#define ADC_READING_TASK_DELAY_MS 500
//

// Typedef

//

// ESP-LOG tasks
const static char* TAG_ADC = "ADC";
//

// Handles
static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc1_cali_handle;
static TaskHandle_t xTaskAdcRead_handle;
//

// Global variables

//

// Function prototypes
void adc1_init();
void adc1_create_oneshot_unit();
void adc1_channel0_config();
void adc1_calibration();
uint16_t get_adc1_c0_voltage_multisampling();
void tasks_create();

void vTaskAdc1C0Read();
//

// Main
void app_main(void){
    adc1_init();
    tasks_create();
}
//

// Functions
void adc1_init(){
    adc1_create_oneshot_unit();
    adc1_channel0_config();
    adc1_calibration();

    ESP_LOGI(TAG_ADC, "ADC1 Initialized");
}


void adc1_create_oneshot_unit(){
    adc_oneshot_unit_init_cfg_t adc1_init_config = {
        .unit_id = ADC_UNIT_1,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc1_init_config, &adc1_handle));
    ESP_LOGI(TAG_ADC, "ADC1 Oneshot created");
}

void adc1_channel0_config(){
    adc_oneshot_chan_cfg_t adc1_c0_config = {
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC1_DESIRED_BITWIDTH
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &adc1_c0_config));
    ESP_LOGI(TAG_ADC, "ADC1 Channel 0 configured");
}

void adc1_calibration(){
    adc_cali_line_fitting_config_t adc1_cali_config = {
            .unit_id = ADC_UNIT_1,
            .atten = ADC_ATTEN_DB_11,
            .bitwidth = ADC1_DESIRED_BITWIDTH
    };

    ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&adc1_cali_config, &adc1_cali_handle));
    ESP_LOGI(TAG_ADC, "ADC1 Calibration completed");
}

uint16_t get_adc1_c0_voltage_multisampling(){
    uint16_t adc_raw = 0;
    uint16_t adc_raw_sum = 0;
    uint16_t adc_voltage = 0;

    for(int i = 0; i < MULTISAMPLE_SIZE; i++){
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &adc_raw);
        adc_raw_sum += adc_raw;
        vTaskDelay(pdMS_TO_TICKS(MULTISAMPLE_DELAY_MS));
    }
    adc_raw_sum /= MULTISAMPLE_SIZE;
        
    adc_cali_raw_to_voltage(adc1_cali_handle, adc_raw, &adc_voltage);
    
    return adc_voltage;
}
//

// FreeRTOS Task create
void tasks_create(){
    xTaskCreate(vTaskAdc1C0Read,
                "ADC1 C0 read task",
                configMINIMAL_STACK_SIZE,
                NULL,
                tskIDLE_PRIORITY + 1,
                &xTaskAdcRead_handle);
}
//

// FreeRTOS Tasks
void vTaskAdc1C0Read(){
    uint16_t adc_voltage = 0;
    while(true){
        adc_voltage = get_adc1_c0_voltage_multisampling();
        ESP_LOGI(TAG_ADC, "ADC1-C0: %d mV", adc_voltage);

        vTaskDelay(pdMS_TO_TICKS(ADC_READING_TASK_DELAY_MS));
    }
}
//
