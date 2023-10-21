#ifndef DATE_TIME_H
#define DATE_TIME_H

#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void setup_time();
void create_time_task();
void set_time(int time);
char* get_time();

#endif