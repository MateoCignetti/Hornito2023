#include "power.h"
#define SAMPLING_FREQUENCY 2500 // Sampling frequency in Hz
#define SIGNAL_FREQUENCY 50     // Frequency of the signal to sample in Hz
#define SAMPLES_AMMOUNT (SAMPLING_FREQUENCY / SIGNAL_FREQUENCY) // Number of samples to take
#define SAMPLING_PERIOD_US (1000000 / SAMPLING_FREQUENCY)   // Sampling period in microseconds
#define PIN_IN GPIO_NUM_34

#define SAMPLE_READING_DELAY_MS 25

#define VOLTAGE_DIVIDER_RATIO 6.86
#define DIODE_FORWARD_VOLTAGE 1.1
#define TRANSORMER_RATIO 16.58

// Global variables
static const char* TAG_POWER = "POWER";

static int current_samples = 0;         // Current sample index
static float samples[SAMPLES_AMMOUNT];    // Array of samples
static esp_timer_handle_t sampling_timer_handle = NULL;
static bool samples_taken = false;

QueueHandle_t xQueuePower;
SemaphoreHandle_t xSemaphorePower;
//

// Function prototypes
void create_sampling_timer();
void sampling_timer_callback();
void create_power_tasks();
void vTaskPower(void *pvParameters);
void scale_samples();
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

void scale_samples(){
    for(int i = 0; i < SAMPLES_AMMOUNT; i++){
        samples[i] /= 1000.0;
        if(samples[i] < 0.15){
            samples[i] = 0;
        } else{
            samples[i] = (samples[i] * VOLTAGE_DIVIDER_RATIO + DIODE_FORWARD_VOLTAGE) * TRANSORMER_RATIO;
        }
        printf("%.2f\n",samples[i]);
    }
}

float getVrms(int delay_steps){
    float sum = 0;

    for(int i = 0; i < SAMPLES_AMMOUNT; i++){
        if(i < delay_steps || (i > (SAMPLES_AMMOUNT/2) && i < (SAMPLES_AMMOUNT/2) + delay_steps)){
            continue;
        } else{
            printf("%d\n", i);
            sum += pow(samples[i], 2);
        }
    }
    printf("Sum: %.2f\n", sum);
    sum /= SAMPLES_AMMOUNT;
    return sqrt(sum);
}

void create_power_tasks(){
    xTaskCreatePinnedToCore(&vTaskPower, "Power read task", 2048, NULL, 5, NULL, 0);
}

void vTaskPower(void *pvParameters){
    xQueuePower = xQueueCreate(1, sizeof(float));
    xSemaphorePower = xSemaphoreCreateBinary();

    while(true){
        if(xSemaphoreTake(xSemaphorePower, portMAX_DELAY)){
            samples_taken = false;
            ESP_LOGI(TAG_POWER, "Taking samples\n");
            create_sampling_timer();
            while(!samples_taken){
                vTaskDelay(SAMPLE_READING_DELAY_MS);
            }
            scale_samples();
            float vrms = getVrms(20);
            if(xQueueSend(xQueuePower, &vrms, portMAX_DELAY) != pdPASS){
                ESP_LOGE(TAG_POWER, "Error sending vrms to queue\n");
            }
        }
    }

    scale_samples();
    printf("Vrms: %.2f\n", getVrms(20));
    vTaskDelete(NULL);
}
//