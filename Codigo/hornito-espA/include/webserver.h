#ifndef WEB_SERVER_H
#define WEB_SERVER_H

// Includes
#include <string.h>
#include "esp_http_server.h"
#include "date_time.h"
#include "cJSON.h"
#include "adc.h"
#include "ntc.h"
#include "freertos/queue.h"
#include "power.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"

//

// Public function prototypes
void start_webserver();
//

#endif