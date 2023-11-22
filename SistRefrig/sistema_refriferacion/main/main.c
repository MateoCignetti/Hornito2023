#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Define los pines para las placas Peltier y la bomba
#define PIN_PELTIER_1 
#define PIN_PELTIER_2 
#define PIN_PELTIER_3 
#define PIN_PUMP 

#define SENSING_TIME 5000 //Tiempo de sensado 

// Función para leer la temperatura del sensor (debe ser implementada)
/*
float getTemperature() {
}
*/

void decisionTask(void *pvParameters) {

    gpio_set_direction(PIN_PELTIER_1, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_PELTIER_2, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_PELTIER_3, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_PUMP, GPIO_MODE_OUTPUT);

    while (1) {
        float temperature = getTemperature();

        if (temperature >= 14.0) {
            // Enciende las tres placas Peltier y la bomba
            gpio_set_level(PIN_PELTIER_1, 1);
            gpio_set_level(PIN_PELTIER_2, 1);
            gpio_set_level(PIN_PELTIER_3, 1);
            gpio_set_level(PIN_PUMP, 1);
        } else if (temperature <= 12.0) {
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
        vTaskDelay(pdMS_TO_TICKS(SENSING_TIME));
    }
}

void app_main() {

    xTaskCreate(&decisionTask,
                "Decision Task", 
                configMINIMAL_STACK_SIZE,
                NULL,
                tskIDLE_PRIORITY + 1,
                NULL,
                0);
}
