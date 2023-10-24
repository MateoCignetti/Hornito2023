#ifndef PELTIER_H
#define PELTIER_H

#include "driver/gpio.h"
#include "esp_log.h"
#include "adc.h"
#include "ntc.h"

void create_peltier_task();

#endif