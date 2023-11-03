#include "peltier.h"

#define PIN_PELTIER_1 GPIO_NUM_32
#define PIN_PELTIER_2 GPIO_NUM_33
#define PIN_PELTIER_3 GPIO_NUM_25
#define PIN_PUMP GPIO_NUM_26

#define ADC_UNIT ADC_UNIT_1
#define ADC_CHANNEL ADC_CHANNEL_0

#define SENSING_TIME_MS 10000

static void vTaskDecision(void *pvParameters);

void create_peltier_task(){
    xTaskCreatePinnedToCore(vTaskDecision,
                            "Decision Task", 
                            configMINIMAL_STACK_SIZE * 5,
                            NULL,
                            tskIDLE_PRIORITY + 1,
                            NULL,
                            0);
}

static void vTaskDecision(void *pvParameters) {

    gpio_set_direction(PIN_PELTIER_1, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_PELTIER_2, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_PELTIER_3, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_PUMP, GPIO_MODE_OUTPUT);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        float temperature_c = get_ntc_temperature_c(get_adc_voltage_mv(ADC_UNIT, ADC_CHANNEL));

        if (temperature_c >= 14.0) {
            // Enciende las tres placas Peltier y la bomba
            gpio_set_level(PIN_PELTIER_1, 1);
            gpio_set_level(PIN_PELTIER_2, 1);
            gpio_set_level(PIN_PELTIER_3, 1);
            gpio_set_level(PIN_PUMP, 1);
        } else if (temperature_c <= 12.0) {
            // Apaga las tres placas Peltier y la bomba
            gpio_set_level(PIN_PELTIER_1, 0);
            gpio_set_level(PIN_PELTIER_2, 0);
            gpio_set_level(PIN_PELTIER_3, 0);
            gpio_set_level(PIN_PUMP, 0);
        } else {
            // Enciende una placa Peltier y la bomba
            gpio_set_level(PIN_PELTIER_1, 1);
            gpio_set_level(PIN_PELTIER_2, 0);
            gpio_set_level(PIN_PELTIER_3, 0);
            gpio_set_level(PIN_PUMP, 1);
        }

        // Espera un tiempo antes de volver a leer la temperatura
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SENSING_TIME_MS));
    }
}