#include "date_time.h"

#define TIME_DELAY_MS 1000
#define CUSTOM_TIME 1632176400

void setup_time(){
    
    setenv("TZ", "America/Argentina/Cordoba", 1);
    tzset();

}

void time_task(){
    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;

    struct timeval custom_tv = { .tv_sec = CUSTOM_TIME };
    settimeofday(&custom_tv, NULL);

    while(true){
        
        time(&now);
        localtime_r(&now, &timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        printf("The current date/time in Cordoba is: %s\n", strftime_buf);
        vTaskDelay(pdMS_TO_TICKS(TIME_DELAY_MS));
    }
}

void create_time_task(){
    xTaskCreate(time_task, "time_task", 4096, NULL, 5, NULL);
}