#ifndef WEB_SERVER_H
#define WEB_SERVER_H

// Includes
#include <string.h>
#include "esp_http_server.h"
#include "date_time.h"
#include "cJSON.h"
#include "adc.h"
#include "ntc.h"
//

// Public function prototypes
httpd_handle_t start_webserver();
//

#endif