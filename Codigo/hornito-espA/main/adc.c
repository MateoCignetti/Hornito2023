#include "adc.h"

// Defines
#define ADC1_DESIRED_BITWIDTH ADC_BITWIDTH_12   // Defines the desired bitwidth for ADC1
#define ADC2_DESIRED_BITWIDTH ADC_BITWIDTH_12   // Defines the desired bitwidth for ADC2
#define ADC1_CONFIGURED_CHANNELS 4              // Defines the number of channels to be configured on ADC1
#define ADC2_CONFIGURED_CHANNELS 0              // Defines the number of channels to be configured on ADC2
#define MULTISAMPLE_SIZE 64                     // Defines the number of ADC readings to be taken in the multisampling function
#define MULTISAMPLE_DELAY_MS 1                  // Defines the delay between each ADC reading in the multisampling function
#define ADC_READING_TASK_DELAY_MS 2000          // Must not be less than (MULTISAMPLE_SIZE * MULTISAMPLE_DELAY_MS)
#define ADC_DEBUGGING_TASK_DELAY_MS 5000        // Defines the period of the ADC debugging task

#define ADC_DEBUGGING_TASK 0                    // Defines and creates a task that reads all configured channels every 5 seconds

// ESP-LOG Tags
const static char* TAG_ADC = "ADC";
//

// Handles
static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc1_cali_handle;
static adc_oneshot_unit_handle_t adc2_handle;
static adc_cali_handle_t adc2_cali_handle;
#if ADC_DEBUGGING_TASK
static TaskHandle_t xTaskAdcDebug_handle;
#endif
//

// Private function prototypes
static void adc1_create_oneshot_unit();
static void adc1_calibration();
static void adc1_configure_channels();
static void adc2_create_oneshot_unit();
static void adc2_calibration();
static void adc2_configure_channels();
#if ADC_DEBUGGING_TASK
static void vTaskReadAllChannels();
#endif
//

// Functions
void adc1_init(){
    if(ADC1_CONFIGURED_CHANNELS > 0){
        adc1_create_oneshot_unit();
        adc1_calibration();
        adc1_configure_channels();

        ESP_LOGI(TAG_ADC, "ADC1 Initialized");
    } else{
        ESP_LOGI(TAG_ADC, "ADC1 not initialized, no channels configured");
    }
}

void adc2_init(){
    if(ADC2_CONFIGURED_CHANNELS > 0){
        adc2_create_oneshot_unit();
        adc2_calibration();
        adc2_configure_channels();

        ESP_LOGI(TAG_ADC, "ADC2 Initialized");
    } else{
        ESP_LOGI(TAG_ADC, "ADC2 Not initialized, no channels configured");
    }
}


static void adc1_create_oneshot_unit(){
    adc_oneshot_unit_init_cfg_t adc1_init_config = {
        .unit_id = ADC_UNIT_1,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc1_init_config, &adc1_handle));
    ESP_LOGI(TAG_ADC, "ADC1 Oneshot created");
}

static void adc2_create_oneshot_unit(){
    adc_oneshot_unit_init_cfg_t adc2_init_config = {
        .unit_id = ADC_UNIT_2,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc2_init_config, &adc2_handle));
    ESP_LOGI(TAG_ADC, "ADC2 Oneshot created");
}

static void adc1_calibration(){
    adc_cali_line_fitting_config_t adc1_cali_config = {
            .unit_id = ADC_UNIT_1,
            .atten = ADC_ATTEN_DB_11,
            .bitwidth = ADC1_DESIRED_BITWIDTH
    };

    ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&adc1_cali_config, &adc1_cali_handle));
    ESP_LOGI(TAG_ADC, "ADC1 Calibration completed");
}

static void adc2_calibration(){
    adc_cali_line_fitting_config_t adc2_cali_config = {
            .unit_id = ADC_UNIT_2,
            .atten = ADC_ATTEN_DB_11,
            .bitwidth = ADC2_DESIRED_BITWIDTH
    };

    ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&adc2_cali_config, &adc2_cali_handle));
    ESP_LOGI(TAG_ADC, "ADC2 Calibration completed");
}

static void adc1_configure_channels(){
    adc_oneshot_chan_cfg_t adc1_channel_configs[ADC1_CONFIGURED_CHANNELS];
    for(int i = 0; i < ADC1_CONFIGURED_CHANNELS; i++){
        adc1_channel_configs[i].atten = ADC_ATTEN_DB_11;
        adc1_channel_configs[i].bitwidth = ADC1_DESIRED_BITWIDTH;

        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, i, &adc1_channel_configs[i]));
        ESP_LOGI(TAG_ADC, "ADC1 Channel %d configured", i);
    }
}

static void adc2_configure_channels(){
    adc_oneshot_chan_cfg_t adc2_channel_configs[ADC2_CONFIGURED_CHANNELS];
    for(int i = 0; i < ADC2_CONFIGURED_CHANNELS; i++){
        adc2_channel_configs[i].atten = ADC_ATTEN_DB_11;
        adc2_channel_configs[i].bitwidth = ADC2_DESIRED_BITWIDTH;

        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, i, &adc2_channel_configs[i]));
        ESP_LOGI(TAG_ADC, "ADC2 Channel %d configured", i);
    }
}

uint16_t get_adc_voltage_mv_multisampling(adc_unit_t adc_unit, adc_channel_t adc_channel){
    adc_oneshot_unit_handle_t adc_handle = NULL;
    adc_cali_handle_t adc_cali_handle = NULL;
    if(adc_unit == ADC_UNIT_1 && adc_channel < ADC1_CONFIGURED_CHANNELS){
        adc_handle = adc1_handle;
        adc_cali_handle = adc1_cali_handle;
    } else if(adc_unit == ADC_UNIT_2 && adc_channel < ADC2_CONFIGURED_CHANNELS){
        adc_handle = adc2_handle;
        adc_cali_handle = adc2_cali_handle;
    } else{
        ESP_LOGE(TAG_ADC, "Invalid ADC unit or channel");
        return 0;
    }

    uint16_t adc_raw = 0;
    uint32_t adc_raw_sum = 0;
    uint16_t adc_voltage = 0;
    
    for(int i = 0; i < MULTISAMPLE_SIZE; i++){
        adc_oneshot_get_calibrated_result(adc_handle, adc_cali_handle, adc_channel, &adc_raw);
        adc_raw_sum += adc_raw;
        vTaskDelay(pdMS_TO_TICKS(MULTISAMPLE_DELAY_MS));
    }

    adc_voltage = adc_raw_sum / MULTISAMPLE_SIZE;
    
    return adc_voltage;
}

void create_adc_tasks(){
    #if ADC_DEBUGGING_TASK
    xTaskCreatePinnedToCore(vTaskReadAllChannels,
                            "ADC1 C0 read task",
                            configMINIMAL_STACK_SIZE * 10,
                            NULL,
                            tskIDLE_PRIORITY + 1,
                            &xTaskAdcDebug_handle,
                            0);
    #endif
}
//

// FreeRTOS Tasks
#if ADC_DEBUGGING_TASK
static void vTaskReadAllChannels(){  //Only used for debugging
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while(true){
        for(int i = 0; i < ADC1_CONFIGURED_CHANNELS; i++){
            uint16_t adc_raw = 0;
            uint16_t adc_voltage = 0;
            adc_oneshot_read(adc1_handle, i, &adc_raw);
            adc_cali_raw_to_voltage(adc1_cali_handle, adc_raw, &adc_voltage);
            ESP_LOGI(TAG_ADC, "ADC1-C%d: %d mV", i, adc_voltage);
        }uint16_t

        for(int i = 0; i < ADC2_CONFIGURED_CHANNELS; i++){
            uint16_t adc_raw = 0;
            uint16_t adc_voltage = 0;
            adc_oneshot_read(adc2_handle, i, &adc_raw);
            adc_cali_raw_to_voltage(adc2_cali_handle, adc_raw, &adc_voltage);
            ESP_LOGI(TAG_ADC, "ADC2-C%d: %d mV", i, adc_voltage);
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(ADC_DEBUGGING_TASK_DELAY_MS));
    }
}
#endif
//