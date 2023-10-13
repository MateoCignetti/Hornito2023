#include "date_time.h"

#define TIME_DELAY_MS 1000
#define CUSTOM_TIME 1695687516

void setup_time(){
    setenv("TZ", "WART4WARST,J1/0,J365/25", 1);
    tzset();
}

void time_task(){
    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;

    struct timeval custom_tv = { .tv_sec = CUSTOM_TIME };
    settimeofday(&custom_tv, NULL);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while(true){
        
        time(&now);
        localtime_r(&now, &timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        printf("The current date/time in Cordoba is: %s\n", strftime_buf);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TIME_DELAY_MS));
    }
}

void set_time(int time){
    time_t now = time;
    struct timeval custom_tv = { .tv_sec = time };
    settimeofday(&custom_tv, NULL);
}

void create_time_task(){
    xTaskCreate(time_task, "time_task", 4096, NULL, 5, NULL);
}