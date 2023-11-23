#include "date_time.h"

#define TIME_DELAY_MS 1000
#define CUSTOM_TIME 1695687516  //Custom time value to set the system time.
#define TIME_ARRAY_SIZE 100

void setup_time(){
    setenv("TZ", "WART4WARST,J1/0,J365/25", 1); //Timezone setting for "WART4WARST" (Argentina Time).
    tzset();                                    //Update environment variables related to timezone.
}

//Task for time synchronization and printing date and time.
void time_task(){
    time_t now;             //Current time in seconds since January 1, 1970 (Unix epoch).
    char strftime_buf[64];  //Buffer to store formatted date and time as a string.
    struct tm timeinfo;     //Structure to store broken-down date and time.

    struct timeval custom_tv = { .tv_sec = CUSTOM_TIME }; //Define timeval structure with "CUSTOM_TIME".
    settimeofday(&custom_tv, NULL);                       //Set the current time using timeval structure.
    TickType_t xLastWakeTime = xTaskGetTickCount();       //Get current task execution time.

    while(true){
        time(&now);                                                         //Get current time in seconds.
        localtime_r(&now, &timeinfo);                                       //Convert current time to local tm structure.
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);      //Format tm structure into a string.
        printf("The current date/time in Cordoba is: %s\n", strftime_buf);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TIME_DELAY_MS));      //Delay until the next execution.
    }
}

void set_time(int time){
    time_t now = time;                              //Convert the time value to time_t type.
    struct timeval custom_tv = { .tv_sec = now };  //Define timeval structure with specified time.
    settimeofday(&custom_tv, NULL);                 //Set the current time using timeval structure.
}

char* get_time(){
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    char *strftime_buf = (char*)malloc(TIME_ARRAY_SIZE);
    strftime(strftime_buf, TIME_ARRAY_SIZE, "%c", &timeinfo);
    
    return strftime_buf;
}

void create_time_task(){
    xTaskCreate(time_task, "time_task", 4096, NULL, 5, NULL);   //Create the "time_task" task with a stack size of 4096 words and priority 5.
}