#ifndef CONTROL_SYSTEM_H
#define CONTROL_SYSTEM_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "esp_log.h"
#include "dimmer.h"
#include "ntc.h"

void create_control_system_task();

#endif