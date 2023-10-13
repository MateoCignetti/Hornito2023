#include "date_time.h"

#define TIME_DELAY_MS 1000
#define CUSTOM_TIME 1695687516

void setup_time(){
    setenv("TZ", "WART4WARST,J1/0,J365/25", 1); //Zona horaria "WART4WARST" (Hora de Argentina).
    tzset();                                    // Actualiza las variables de entorno relacionadas con la zona horaria.
}

//Task para sincronización del tiempo e impresión de fecha y hora.
void time_task(){
    time_t now;             //Tiempo actual en segundos desde el 1 de enero de 1970 (Unix epoch)
    char strftime_buf[64];  //Búfer para almacenar la fecha y hora formateadas como una cadena de caracteres.
    struct tm timeinfo;     //Estructura para almacenar la fecha y hora desglosadas

    struct timeval custom_tv = { .tv_sec = CUSTOM_TIME }; //Estructura timeval con el tiempo "CUSTOM_TIME".
    settimeofday(&custom_tv, NULL);                       //Establece la hora actual usando la estructura timeval.
    TickType_t xLastWakeTime = xTaskGetTickCount();       //Obtiene el tiempo de ejecución actual de la tarea.

    while(true){
        time(&now);                                                         //Obtención del tiempo actual en segundos.
        localtime_r(&now, &timeinfo);                                       //Convierte el tiempo actual en una estructura tm local.
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);      //Formatea la estructura tm en una cadena de caracteres.
        printf("The current date/time in Cordoba is: %s\n", strftime_buf);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TIME_DELAY_MS));
    }
}

void set_time(int time){
    time_t now = time;                              
    struct timeval custom_tv = { .tv_sec = time };  //Estructura timeval con el tiempo especificado.
    settimeofday(&custom_tv, NULL);                 //Establece la hora actual usando la estructura timeval
}

void create_time_task(){
    xTaskCreate(time_task, "time_task", 4096, NULL, 5, NULL);
}