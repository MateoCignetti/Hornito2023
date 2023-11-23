#include "ntc.h"

// Defines
#define NTC_READ_TASK_DELAY_MS 1000     // Defines the period of the NTC reading task

// Constant values
//const int DIVIDER_RESISTOR_O = 46690;   // Resistance in Ohms of the resistor in series with the NTC
const int DIVIDER_RESISTOR_O = 9900;
const float SUPPLY_VOLTAGE_V = 3.324;   // Supply voltage in Volts
//const float A_NTC = 0.1609525156;       // Steinhart-Hart coefficient A
const float A_NTC = 0.006543521;
//const float B_NTC = 3977.1932;          // Steinhart-Hart coefficient B
const float B_NTC = 4245.542776;

// Handles
static TaskHandle_t xTaskAdcDebug_handle;

// ESP-LOG Tags
const static char* TAG_NTC = "NTC";

// Private function prototypes
static void vTaskReadNTCs();

// Functions
float get_ntc_temperature_c(uint16_t adc_reading_mv){
    float adc_voltage_v = adc_reading_mv / 1000.0;
    //float ntc_resistance_ko = (DIVIDER_RESISTOR_O / 1000 * ((SUPPLY_VOLTAGE_V * 1000) - adc_reading_mv)) / adc_reading_mv;    //Only used for debugging
    //printf("NTC resistance: %.2f\n kOhm", ntc_resistance_ko);                                                                 //Only used for debugging
    float ntc_temperature_c = (B_NTC / log((DIVIDER_RESISTOR_O* ( (SUPPLY_VOLTAGE_V / adc_voltage_v) - 1) ) / A_NTC)) - 273.15;
    return ntc_temperature_c;
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
    TickType_t xLastWakeTime;
    while(true){
        ESP_LOGI(TAG_NTC, "NTC1: %.2f °C", get_ntc_temperature_c(get_adc_voltage_mv_multisampling(ADC_UNIT_1, ADC_CHANNEL_0)));
        vTaskDelayUntil(&xLastWakeTime, NTC_READ_TASK_DELAY_MS / portTICK_PERIOD_MS);
    }
}
