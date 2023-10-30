#include "power.h"
#define SAMPLING_FREQUENCY 10000 // Hz
#define SAMPLES_AMMOUNT 100 // SAMPLING_FREQUENCY / Signal frequency

static int current_samples = 0;
static int samples[SAMPLES_AMMOUNT];

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
    esp_timer_start_periodic(sampling_timer_handle, 1 / SAMPLING_FREQUENCY);
}

void sampling_timer_callback(){
    samples[current_samples] = get_adc_voltage_mv(ADC_UNIT_1, ADC_CHANNEL_0);
    current_samples ++;
    if(current_samples == SAMPLES_AMMOUNT){
        unsigned int sum = 0;
        for(int i = 0; i < SAMPLES_AMMOUNT; i++){
            sum += pow(samples[i], 2) * (1/SAMPLING_FREQUENCY);
        }
        float vrms = sqrt(SAMPLING_FREQUENCY * sum);
        printf("ANASHE: %.2f mV\n", vrms);
        current_samples = 0;
    }
}