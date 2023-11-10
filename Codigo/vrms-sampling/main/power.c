#include "power.h"
#define SAMPLING_FREQUENCY 2000 // Sampling frequency in Hz
#define SIGNAL_FREQUENCY 50     // Frequency of the signal to sample in Hz
#define SAMPLES_AMMOUNT (SAMPLING_FREQUENCY / SIGNAL_FREQUENCY) // Number of samples to take
#define SAMPLING_PERIOD_US (1000000 / SAMPLING_FREQUENCY)   // Sampling period in microseconds
#define PIN_IN GPIO_NUM_34

// Global variables
static int current_samples = 0;         // Current sample index
static float samples[SAMPLES_AMMOUNT];    // Array of samples
static esp_timer_handle_t sampling_timer_handle = NULL;
static SemaphoreHandle_t power_semaphore = NULL;
static TaskHandle_t power_task_handle = NULL;  // Task handle for vTaskPower
//

// Function prototypes


void setup_power_isr();
void zero_crossing_interrupt();
void delete_power_isr();
void create_sampling_timer();
void sampling_timer_callback();
void create_power_tasks();
void vTaskPower();
//

// Functions
void setup_power_isr(){
    gpio_set_direction(PIN_IN, GPIO_MODE_INPUT);
    gpio_set_intr_type(PIN_IN, GPIO_INTR_POSEDGE);
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    gpio_isr_handler_add(PIN_IN, zero_crossing_interrupt, NULL);
    //esp_timer_init();
}

void IRAM_ATTR zero_crossing_interrupt(){
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(power_semaphore, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

void delete_power_isr(){
    gpio_isr_handler_remove(PIN_IN);
}

void create_sampling_timer() {
    delete_power_isr();

    esp_timer_create_args_t sampling_timer_args = {
        .callback = &sampling_timer_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "sampling_timer"
    };

    printf("sampling_period_us: %d\n", SAMPLING_PERIOD_US);
    printf("samples_ammount: %d\n", SAMPLES_AMMOUNT);
    esp_timer_create(&sampling_timer_args, &sampling_timer_handle);
    esp_timer_start_periodic(sampling_timer_handle, SAMPLING_PERIOD_US);
}


void sampling_timer_callback(){
    
    samples[current_samples] = get_adc_voltage_mv(ADC_UNIT_1, ADC_CHANNEL_0);
    //printf("%.2f\n", samples[current_samples]);
    current_samples++;
    if(current_samples == SAMPLES_AMMOUNT){
        for(int i = 0; i < SAMPLES_AMMOUNT; i++){
            samples[i] /= 1000.0;
            if(samples[i] < 0.15){
                samples[i] = 0;
            } else{
                samples[i] = (samples[i] * 6.86 + 1.1) * 16.58;
            }
            printf("%.2f\n",samples[i]);
            //printf("%d\n", samples[i]);
        }
        esp_timer_stop(sampling_timer_handle);
        /*float sum = 0;
        for(int i = 0; i < SAMPLES_AMMOUNT; i++){
            sum += pow(samples[i], 2);
        }
        //printf("%.2f\n", sum);
        float vrms = sqrt(sum / SAMPLES_AMMOUNT);
        printf("ANASHE: %.2f mV\n", vrms);
        current_samples = 0;*/
    }
}

void create_power_tasks(){
    xTaskCreate(vTaskPower, "power_task", 2048, NULL, 1, &power_task_handle);
}

void vTaskPower(){
    power_semaphore = xSemaphoreCreateBinary();
    while(true){
        if(xSemaphoreTake(power_semaphore, portMAX_DELAY)){
            printf("Semaphore taken\n");
            create_sampling_timer();
            vTaskDelete(power_task_handle);
        } else{
            printf("Semaphore timeout\n");}
    }
}

//