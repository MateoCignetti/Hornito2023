#ifndef TASKS_H
#define TASKS_H

#include "adc.h"
#include "dimmer.h"
#include "ntc.h"
#include "control_system.h"
#include "date_time.h"
#include "webserver.h"
#include "wifi.h"
#include "power.h"

void create_tasks();
void delete_tasks();

#endif