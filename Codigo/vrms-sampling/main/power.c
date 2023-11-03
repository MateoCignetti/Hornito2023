#include "power.h"
#define SAMPLING_FREQUENCY 10000 // Sampling frequency in Hz
#define SIGNAL_FREQUENCY 50     // Frequency of the signal to sample in Hz
#define SAMPLES_AMMOUNT (SAMPLING_FREQUENCY / SIGNAL_FREQUENCY) // Number of samples to take
#define SAMPLING_PERIOD_US (1000000 / SAMPLING_FREQUENCY)   // Sampling period in microseconds

static int current_samples = 0;         // Current sample index
static int samples[SAMPLES_AMMOUNT];    // Array of samples

static esp_timer_handle_t sampling_timer_handle;

void sampling_timer_callback();

void create_sampling_timer(){
    esp_timer_init();

    esp_timer_create_args_t sampling_timer_args = {
        .callback = &sampling_timer_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "sampling_timer"
    };
    esp_timer_create(&sampling_timer_args, &sampling_timer_handle);
    esp_timer_start_periodic(sampling_timer_handle, SAMPLING_PERIOD_US);

    printf("sampling_period_us: %d\n", SAMPLING_PERIOD_US);
    printf("samples_ammount: %d\n", SAMPLES_AMMOUNT);
}

void sampling_timer_callback(){
    samples[current_samples] = get_adc_voltage_mv(ADC_UNIT_1, ADC_CHANNEL_0);
    current_samples++;
    if(current_samples == SAMPLES_AMMOUNT){
        float sum = 0;
        for(int i = 0; i < SAMPLES_AMMOUNT; i++){
            sum += pow(samples[i], 2);
        }
        //printf("%.2f\n", sum);
        float vrms = sqrt(sum / SAMPLES_AMMOUNT);
        printf("ANASHE: %.2f mV\n", vrms);
        current_samples = 0;
    }
}