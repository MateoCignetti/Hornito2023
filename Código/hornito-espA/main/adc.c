#include "adc.h"

// Defines
#define ADC1_DESIRED_BITWIDTH ADC_BITWIDTH_12
#define MULTISAMPLE_SIZE 64
#define MULTISAMPLE_DELAY_MS 1
#define ADC_READING_TASK_DELAY_MS 500
//#define DIVIDER_RESISTOR_O 46690
//#define SUPPLY_VOLTAGE_MV 3324

// Constant values
const int DIVIDER_RESISTOR_O = 46690;
const float SUPPLY_VOLTAGE_V = 3.324;
const float A_NTC = 0.1609525156;
const float B_NTC = 3977.1932;

// ESP-LOG Tags
const static char* TAG_ADC = "ADC";
//

// Handles
static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc1_cali_handle;
static TaskHandle_t xTaskAdcRead_handle;
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

// FreeRTOS Tasks
void vTaskAdc1C0Read(){
    uint16_t adc_voltage_mv = 0;
    float adc_voltage_v = 0.0;
    float ntc_resistance_ko = 0.0;
    float ntc_temperature_c = 0.0;
    while(true){
        adc_voltage_mv = get_adc1_c0_voltage_multisampling();
        adc_voltage_v = adc_voltage_mv / 1000.0;
        printf("%.2f\n", adc_voltage_v);
        ntc_resistance_ko = (DIVIDER_RESISTOR_O / 1000 * ((SUPPLY_VOLTAGE_V * 1000) - adc_voltage_mv)) / adc_voltage_mv;
        ntc_temperature_c = (B_NTC / log((DIVIDER_RESISTOR_O* ( (SUPPLY_VOLTAGE_V / adc_voltage_v) - 1) ) / A_NTC)) - 273.15;
        //ntc_temperature_c =(DIVIDER_RESISTOR_O*((SUPPLY_VOLTAGE_V/(adc_voltage_mv/1000)) - 1))/(A_NTC);
        //ntc_temperature_c = log(ntc_temperature_c);
        ESP_LOGI(TAG_ADC, "ADC1-C0: %.2f °C", ntc_temperature_c);
        ESP_LOGI(TAG_ADC, "ADC1-C0: %.2f kOhm", ntc_resistance_ko);

        
 
        vTaskDelay(pdMS_TO_TICKS(ADC_READING_TASK_DELAY_MS));
    }
}
//