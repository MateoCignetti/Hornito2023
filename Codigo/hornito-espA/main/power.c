#include "power.h"

// Defines
#define SAMPLING_FREQUENCY 2500                                 // Sampling frequency in Hz
#define SIGNAL_FREQUENCY 50                                     // Frequency of the signal to sample in Hz
#define SAMPLES_AMMOUNT (SAMPLING_FREQUENCY / SIGNAL_FREQUENCY) // Number of samples to take
#define SAMPLING_PERIOD_US (1000000 / SAMPLING_FREQUENCY)       // Sampling period in microseconds
#define SAMPLE_READING_DELAY_MS 1                              // Delay between samples in milliseconds

#define VOLTAGE_DIVIDER_RATIO 6.86                              // Voltage divider ratio (Peak voltage before voltage divider
                                                                //                        / Peak voltage after)

#define DIODE_FORWARD_VOLTAGE 1.1                               // Forward voltage drop of two diodes in series (because of
                                                                // the full bridge rectifier)

#define TRANSORMER_RATIO 16.58                                  // Transformer ratio (Vprimary / Vsecondary)
#define INTERIOR_RESISTOR_O 33.5                                // Interior resistor in Ohms
//#define EXTERIOR_RESISTOR_O 217.5                             // Exterior resistor in Ohms
#define RECEIVE_STEPS_TIMEOUT_MS 5000                           // Timeout for receiving steps from control system in milliseconds
#define ZERO_VOLTAGE_LOW_THRESHOLD 120.0
#define ZERO_VOLTAGE_HIGH_THRESHOLD 150.0
//

// Global variables
static const char* TAG_POWER = "POWER";                         // Tag for logging
static int current_samples = 0;                                 // Samples counter for the sampling timer
static float samples[SAMPLES_AMMOUNT];                          // Array to store the samples
static bool samples_taken = false;                              // Flag to indicate if the samples were taken
static esp_timer_handle_t sampling_timer_handle = NULL;         // Handle for the sampling timer     
SemaphoreHandle_t xSemaphorePower;                              // Semaphore to indicate that the power value should be read
QueueHandle_t xQueuePower;                                      // Queue to send the power value
static TaskHandle_t xTaskPower_handle = NULL;                   // Handle for the power task
//

// Function prototypes
void create_sampling_timer();
void sampling_timer_callback();
void scale_samples();
float getVrms(int delay_steps);
void create_power_tasks();
void vTaskPower();
//

// General functions
void create_sampling_timer() {
    esp_timer_create_args_t sampling_timer_args = {
        .callback = &sampling_timer_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "sampling_timer"
    };
    current_samples = 0;

    esp_timer_create(&sampling_timer_args, &sampling_timer_handle);
    esp_timer_start_periodic(sampling_timer_handle, SAMPLING_PERIOD_US);
}


void sampling_timer_callback(){
    taskENTER_CRITICAL();
    samples[current_samples] = get_adc_voltage_mv(ADC_UNIT_1, ADC_CHANNEL_0);
    taskEXIT_CRITICAL();

    if(current_samples == 0){
        if(samples[0] > ZERO_VOLTAGE_LOW_THRESHOLD && samples[0] < ZERO_VOLTAGE_HIGH_THRESHOLD){
            current_samples++;
        }
    } else{
        current_samples++;
    }

    if(current_samples == SAMPLES_AMMOUNT){;
        if(samples[current_samples-1] < (142.0+60.0)){
            ESP_LOGI(TAG_POWER, "Samples taken");
            samples_taken = true;
            esp_timer_stop(sampling_timer_handle);
        } else{
            current_samples = 0;
        }
    }
}

void scale_samples(){
    for(int i = 0; i < SAMPLES_AMMOUNT; i++){
        samples[i] /= 1000.0;
        if(samples[i] < 0.15){
            samples[i] = 0;
        } else{
            samples[i] = (samples[i] * VOLTAGE_DIVIDER_RATIO + DIODE_FORWARD_VOLTAGE) * TRANSORMER_RATIO;
        }
    }
}

float getVrms(int delay_steps){
    float sum = 0;

    for(int i = 0; i < SAMPLES_AMMOUNT; i++){
        if(i < delay_steps || (i > (SAMPLES_AMMOUNT/2) && i < (SAMPLES_AMMOUNT/2) + delay_steps)){
            continue;
        } else{
            sum += pow(samples[i], 2);
        }
    }
    sum /= SAMPLES_AMMOUNT;
    return sqrt(sum);
}
//

// Task-related functions
void create_power_tasks(){
    xTaskCreatePinnedToCore(&vTaskPower, "Power read task", 2048, &xTaskPower_handle, 5, NULL, 0);
}

void delete_power_tasks(){
    vTaskDelete(xTaskPower_handle);
}

void vTaskPower(){
    xQueuePower = xQueueCreate(1, sizeof(float));
    xSemaphorePower = xSemaphoreCreateBinary();
    TickType_t xLastWakeTime = xTaskGetTickCount();
    int delay_steps = 0;

    ESP_LOGI(TAG_POWER, "Sampling period in microseconds: %d", SAMPLING_PERIOD_US);
    ESP_LOGI(TAG_POWER, "Ammount of samples per period: %d", SAMPLES_AMMOUNT);

    while(true){
        if(xSemaphoreTake(xSemaphorePower, portMAX_DELAY)){
            samples_taken = false;

            ESP_LOGI(TAG_POWER, "Taking samples\n");
            create_sampling_timer();
            while(!samples_taken){
                vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SAMPLE_READING_DELAY_MS));
            }
            scale_samples();
            xSemaphoreGive(xSemaphoreControlSystemToPower);
            if(xQueueReceive(xQueueControlSystemToPower, &delay_steps, portMAX_DELAY) != pdPASS){
                ESP_LOGE(TAG_POWER, "Error receiving delay steps from control system\n");
            } else{
                float power = (pow(getVrms(delay_steps),2) ) / INTERIOR_RESISTOR_O;
                if(xQueueSend(xQueuePower, &power, pdMS_TO_TICKS(RECEIVE_STEPS_TIMEOUT_MS)) != pdPASS){
                    ESP_LOGE(TAG_POWER, "Error sending power value to queue\n");
                } else{
                    ESP_LOGI(TAG_POWER, "Sent %.2f W value to power queue", power);
                }
            }
        }
    }

    vTaskDelete(NULL); // Deletes the task in case the while loop breaks
}
//