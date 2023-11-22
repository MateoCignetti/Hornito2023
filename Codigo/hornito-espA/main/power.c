#include "power.h"

// Defines
#define SAMPLING_FREQUENCY 2500                                     // Sampling frequency in Hz
#define SIGNAL_FREQUENCY 50                                         // Frequency of the signal to sample in Hz
#define SAMPLES_AMMOUNT (SAMPLING_FREQUENCY / SIGNAL_FREQUENCY)     // Number of samples to take
#define SAMPLING_PERIOD_US (1000000 / SAMPLING_FREQUENCY)           // Sampling period in microseconds
#define SAMPLE_READING_DELAY_MS 1                                   // Delay between samples in milliseconds

#define VOLTAGE_DIVIDER_RATIO 6.86                                  // Voltage divider ratio (Peak voltage before voltage divider
                                                                    //                        / Peak voltage after)

#define DIODE_FORWARD_VOLTAGE 1.1                                   // Forward voltage drop of two diodes in series (because of
                                                                    // the full bridge rectifier)

#define TRANSORMER_RATIO 16.58                                      // Transformer ratio (Vprimary / Vsecondary)
#define INTERIOR_RESISTOR_O 33.5                                    // Interior resistor in Ohms
//#define EXTERIOR_RESISTOR_O 217.5                                 // Exterior resistor in Ohms
#define RECEIVE_STEPS_TIMEOUT_MS 5000                               // Timeout for receiving steps from control system in milliseconds
#define ZERO_VOLTAGE_LOW_THRESHOLD 120.0                            // Low value threshold for a to be considered 0 mV
#define ZERO_VOLTAGE_HIGH_THRESHOLD 150.0                           // High value threshold for a sample to be considered 0 mV
#define LAST_SAMPLE_THRESHOLD (ZERO_VOLTAGE_HIGH_THRESHOLD + 45.0)  // Threshold for the last sample to be considered correct
#define ADC_UNIT ADC_UNIT_1                                         // ADC unit to use
#define ADC_CHANNEL ADC_CHANNEL_0                                   // ADC channel to use

#define POWER_MONITORING_TASK 0                                     // Enables the power monitoring task
#define MONITORING_DELAY_MS 2000                                    // Delay between power readings in milliseconds
//

// Global variables
static const char* TAG_POWER = "POWER";                             // Tag for logging
static int current_samples = 0;                                     // Samples counter for the sampling timer
static float samples[SAMPLES_AMMOUNT];                              // Array to store the samples
static bool samples_taken = false;                                  // Flag to indicate if the samples were taken
static esp_timer_handle_t sampling_timer_handle = NULL;             // Handle for the sampling timer     
SemaphoreHandle_t xSemaphorePower;                                  // Semaphore to indicate that the power value should be read
QueueHandle_t xQueuePower;                                          // Queue to send the power value
static TaskHandle_t xTaskPower_handle = NULL;                       // Handle for the power task
static SemaphoreHandle_t xSemaphoreSamples = NULL;
#if POWER_MONITORING_TASK
static TaskHandle_t xTaskPowerMonitoring_handle = NULL;             // Handle for the power monitoring task
#endif
//

// Function prototypes
void create_sampling_timer();
void sampling_timer_callback();
static void scale_samples();
static float getVrms(int delay_steps);
void create_power_tasks();
void create_power_semaphores_queues();
static void vTaskPower();
#if POWER_MONITORING_TASK
static void vTaskPowerMonitoring();
#endif
//

// General functions

// Function to create the timer that will take the samples
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

// Callback function for the sampling timer. It takes the samples and stores them in the samples array.
// If the samples are taken correctly (checking that the first sample is 0 mV and
// the last to be below a certain value), it stops the timer.
void sampling_timer_callback(){
    //taskENTER_CRITICAL();
    //vTaskSuspendAll();
    samples[current_samples] = get_adc_voltage_mv(ADC_UNIT, ADC_CHANNEL);
    //taskEXIT_CRITICAL();

    if(current_samples == 0){
        if(samples[0] > ZERO_VOLTAGE_LOW_THRESHOLD && samples[0] < ZERO_VOLTAGE_HIGH_THRESHOLD){
            current_samples++;
        }
    } else{
        current_samples++;
    }

    if(current_samples == SAMPLES_AMMOUNT){;
        if(samples[current_samples-1] < LAST_SAMPLE_THRESHOLD){
            samples_taken = true;
            xSemaphoreGive(xSemaphoreSamples);
            //xTaskResumeAll();
            esp_timer_stop(sampling_timer_handle);
        } else{
            current_samples = 0;
        }
    }
}

// Function to scale the samples from the array to the actual voltage values, before the transformer,
static void scale_samples(){
    for(int i = 0; i < SAMPLES_AMMOUNT; i++){
        samples[i] /= 1000.0;
        if(samples[i] < 0.15){
            samples[i] = 0;
        } else{
            samples[i] = (samples[i] * VOLTAGE_DIVIDER_RATIO + DIODE_FORWARD_VOLTAGE) * TRANSORMER_RATIO;
        }
    }
}

// Function to get the Vrms value from the samples array with a given delay in steps, that should be
// the same steps from the control system.
static float getVrms(int delay_steps){
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

// Function that creates the power-related tasks
void create_power_tasks(){
    xTaskCreatePinnedToCore(&vTaskPower,
                            "Power read task",
                            configMINIMAL_STACK_SIZE * 7,
                            &xTaskPower_handle,
                            tskIDLE_PRIORITY + 5,
                            NULL,
                            0);
    #if POWER_MONITORING_TASK
    xTaskCreatePinnedToCore(&vTaskPowerMonitoring,
                            "Power monitoring task",
                            configMINIMAL_STACK_SIZE * 5,
                            &xTaskPowerMonitoring_handle,
                            tskIDLE_PRIORITY + 1,
                            NULL,
                            0);
    #endif
}

void create_power_semaphores_queues(){
    xQueuePower = xQueueCreate(1, sizeof(float));
    xSemaphorePower = xSemaphoreCreateBinary();
    ESP_LOGI(TAG_POWER, "Power queue and semaphore created");
}

// Function that deletes the power-related tasks
void delete_power_tasks(){
    if(xTaskPower_handle != NULL){
        vTaskDelete(xTaskPower_handle);
        xTaskPower_handle = NULL;
    }

    #if POWER_MONITORING_TASK
    if(xTaskPowerMonitoring_handle != NULL){
        vTaskDelete(xTaskPowerMonitoring_handle);
        xTaskPowerMonitoring_handle = NULL;
    }
    #endif
}

// Main power task. It wait for a semaphore to be released from the webserver when a power value
// must be measured. It then takes the samples, scales them and releases a semaphore for the
// control system task to get a steps delay value. Once the vrms value is calculated, it sends
// the power value to a queue for the webserver to read.
static void vTaskPower(){
    //TickType_t xLastWakeTime = xTaskGetTickCount();
    int delay_steps = 0;
    float power = 0.0;

    ESP_LOGI(TAG_POWER, "Sampling period in microseconds: %d", SAMPLING_PERIOD_US);
    ESP_LOGI(TAG_POWER, "Ammount of samples per period: %d", SAMPLES_AMMOUNT);
    xSemaphoreSamples = xSemaphoreCreateBinary();

    while(true){
        if(xSemaphoreTake(xSemaphorePower, portMAX_DELAY)){
            samples_taken = false;

            ESP_LOGI(TAG_POWER, "Taking samples\n");
            create_sampling_timer();
            if(xSemaphoreTake(xSemaphoreSamples, portMAX_DELAY)){
                ESP_LOGI(TAG_POWER, "Samples taken");
                scale_samples();
                xSemaphoreGive(xSemaphoreControlSystemToPower);
                if(xQueueReceive(xQueueControlSystemToPower, &delay_steps, portMAX_DELAY) != pdPASS){
                    ESP_LOGE(TAG_POWER, "Error receiving delay steps from control system\n");
                } else{
                    if(delay_steps == 0){
                        power = 0.0;
                    } else{
                        power = (pow(getVrms(delay_steps),2) ) / INTERIOR_RESISTOR_O;
                    }
                    if(xQueueSend(xQueuePower, &power, pdMS_TO_TICKS(RECEIVE_STEPS_TIMEOUT_MS)) != pdPASS){
                        ESP_LOGE(TAG_POWER, "Error sending power value to queue\n");
                    } else{
                        ESP_LOGI(TAG_POWER, "Sent %.2f W value to power queue", power);
                    }
                }
            }
        }
    }
}


#if POWER_MONITORING_TASK
// Power monitoring task. It reads the power value from the queue and sends it to the webserver.
static void vTaskPowerMonitoring(){
    while(true){
        ESP_LOGW(TAG_POWER, "Power task stack size: %d", uxTaskGetStackHighWaterMark(xTaskPower_handle));
        vTaskDelay(pdMS_TO_TICKS(MONITORING_DELAY_MS));
    }
}
#endif
//