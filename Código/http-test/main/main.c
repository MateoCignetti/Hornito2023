// Includes
#include "wifi.h"
#include "webserver.h"
//

// Function prototypes

//

// Global variables

//

// Main
void app_main(void){
    setup_wifi();
    httpd_handle_t webserver = start_webserver();
}
//

// Functions


//