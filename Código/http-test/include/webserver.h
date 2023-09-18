#ifndef WEB_SERVER_H
#define WEB_SERVER_H

// Includes
#include "esp_http_server.h"

#include <time.h> // Just for testing purposes
//

// Public function prototypes
httpd_handle_t start_webserver();
//

#endif