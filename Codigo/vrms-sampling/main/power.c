#include "power.h"
#define SAMPLING_FREQUENCY 2500 // Sampling frequency in Hz
#define SIGNAL_FREQUENCY 50     // Frequency of the signal to sample in Hz
#define SAMPLES_AMMOUNT (SAMPLING_FREQUENCY / SIGNAL_FREQUENCY) // Number of samples to take
#define SAMPLING_PERIOD_US (1000000 / SAMPLING_FREQUENCY)   // Sampling period in microseconds
#define PIN_IN GPIO_NUM_34

// Global variables
static int current_samples = 0;         // Current sample index
static float samples[SAMPLES_AMMOUNT];    // Array of samples
static esp_timer_handle_t sampling_timer_handle = NULL;
static bool samples_taken = false;
//

// Function prototypes

void create_sampling_timer();
void sampling_timer_callback();
void create_power_tasks();
void vTaskPower(void *pvParameters);
void print_samples();
//

// Functions


void create_sampling_timer() {
    //esp_timer_init();
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
    current_samples = 0;
}


void sampling_timer_callback(){
    samples[current_samples] = get_adc_voltage_mv(ADC_UNIT_1, ADC_CHANNEL_0);
    //printf("%.2f\n", samples[current_samples]);
    if(current_samples == 0){
        //printf("Current_samples = 0\n");
        if(samples[0] > 120.0 && samples[0] < 150.0){
            //printf("Current_samples = 0 and samples[0] > 120.0 && samples[0] < 150.0\n");
            current_samples++;
        }
    } else{
        //printf("Current_samples != 0\n");
        current_samples++;
    }
    //printf("%.2f\n", samples[current_samples]);
    if(current_samples == SAMPLES_AMMOUNT){;
        if(samples[current_samples-1] < (142.0+60.0)){
            printf("Samples taken\n");
            samples_taken = true;
            esp_timer_stop(sampling_timer_handle);
        } else{
            printf("Samples not taken\n");
            current_samples = 0;
        }
        
    }
}

void print_samples(){
    for(int i = 0; i < SAMPLES_AMMOUNT; i++){
        samples[i] /= 1000.0;
        if(samples[i] < 0.15){
            samples[i] = 0;
        } else{
            samples[i] = (samples[i] * 6.86 + 1.1) * 16.58;
        }
        printf("%.2f\n",samples[i]);
    }
}

void create_power_tasks(){
    xTaskCreate(&vTaskPower, "Power read task", 2048, NULL, 5, NULL);
}

void vTaskPower(void *pvParameters){
    create_sampling_timer();
    while(!samples_taken){
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    print_samples();
    vTaskDelete(NULL);
}
//