#include "adc.h"

// Defines
#define ADC1_DESIRED_BITWIDTH ADC_BITWIDTH_12
#define ADC_READING_TASK_DELAY_MS 1

// ESP-LOG Tags
const static char* TAG_ADC = "ADC";
//

// Handles
static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc1_cali_handle;
static TaskHandle_t xTaskAdcRead_handle;
//



// Functions

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

void adc1_init(){
    adc1_create_oneshot_unit();
    adc1_channel0_config();
    adc1_calibration();

    ESP_LOGI(TAG_ADC, "ADC1 Initialized");
}

uint16_t get_adc1_c0_mv(){
    uint16_t adc_raw = 0;
    uint16_t adc_voltage_mv = 0;

    adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &adc_raw);
        
    adc_cali_raw_to_voltage(adc1_cali_handle, adc_raw, &adc_voltage_mv);
    
    return adc_voltage_mv;
}
//



// FreeRTOS Tasks
void vTaskAdc1C0Read(){
    uint16_t adc_voltage_mv = 0;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while(true){
        adc_voltage_mv = get_adc1_c0_mv();
        printf("%d\n", adc_voltage_mv - 142);

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(ADC_READING_TASK_DELAY_MS));   
    }
}

void create_adc_read_task(){
    xTaskCreatePinnedToCore(vTaskAdc1C0Read,
                            "ADC1 C0 read task",
                            configMINIMAL_STACK_SIZE * 10,
                            NULL,
                            tskIDLE_PRIORITY + 1,
                            &xTaskAdcRead_handle,
                            0);
}
//