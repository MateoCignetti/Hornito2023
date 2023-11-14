#ifndef WEB_SERVER_H
#define WEB_SERVER_H

// Includes
#include <string.h>
#include "esp_http_server.h"

#include "cJSON.h"
#include "freertos/queue.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#include "adc.h"
#include "date_time.h"
#include "control_system.h"
#include "ntc.h"
#include "power.h"


//

// Public function prototypes
void start_webserver();
//

#endif