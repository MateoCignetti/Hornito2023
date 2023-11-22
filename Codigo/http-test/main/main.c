// Includes
#include "wifi.h"
#include "webserver.h"
#include "date_time.h"
//

// Function prototypes

//

// Global variables

//

// Main
void app_main(void){
    setup_time();
    setup_wifi();
    adc1_init();
    create_time_task();
    httpd_handle_t webserver = start_webserver();
}
//

// Functions


//