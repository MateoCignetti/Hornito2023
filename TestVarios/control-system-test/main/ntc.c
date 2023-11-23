#include "ntc.h"

// Defines
#define NTC_READ_TASK_DELAY_MS 2000         // Defines the period of the NTC reading task

// Constant values
const int DIVIDER_RESISTOR_O = 100000;      // Resistance resistor in series with the NTC (Ohm)
const float SUPPLY_VOLTAGE_V = 3.3;         // Supply voltage (V)
const float BETA_NTC = 3813.902828;         // Thermistor beta coefficient (K)
const float T0_NTC = 25 + 273.15;           // Reference temperature (K)           
const float R0_NTC = 100000;                // Reference resistance at reference temperature (Ohm)

// Handles
static TaskHandle_t xTaskAdcDebug_handle;

// ESP-LOG Tags
const static char* TAG_NTC = "NTC";

// Private function prototypes
static void vTaskReadNTCs();

// Functions
float get_ntc_temperature_c(int adc_reading_mv){
    float adc_reading_v = adc_reading_mv / 1000.0;
    //printf("ADC reading: %.2f V\n", adc_reading_v); //Only used for debugging
    float ntc_resistance_o =  ((SUPPLY_VOLTAGE_V * DIVIDER_RESISTOR_O)/adc_reading_v) - DIVIDER_RESISTOR_O; //Calculates the resistance of the NTC in Ohms using the voltage divider formula
    //printf("NTC resistance: %.2f Ohm\n", ntc_resistance_o);  //Only used for debugging
    float num = BETA_NTC * T0_NTC;
    float den = BETA_NTC + (T0_NTC * log(ntc_resistance_o / R0_NTC));       
    return (num/den) - 273.15;
}

void create_ntc_tasks(){
    xTaskCreatePinnedToCore(vTaskReadNTCs,
                            "vTaskReadAllChannels",
                            configMINIMAL_STACK_SIZE * 5,
                            NULL,
                            tskIDLE_PRIORITY + 1,
                            &xTaskAdcDebug_handle,
                            0);
}

static void vTaskReadNTCs(){
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while(true){
        ESP_LOGI(TAG_NTC, "NTC1: %.2f °C", get_ntc_temperature_c(get_adc_voltage_mv_multisampling(ADC_UNIT_1, ADC_CHANNEL_0)));
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(NTC_READ_TASK_DELAY_MS));
    }
}
