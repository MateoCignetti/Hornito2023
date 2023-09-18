#include "webserver.h"

#define HTML_NEG_OFFSET 1

// Private function prototypes
static esp_err_t root_get_handler(httpd_req_t *req);
//

// Global variables
static const httpd_uri_t root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler,
};
//

// Functions
httpd_handle_t start_webserver(){
    httpd_handle_t httpd_server = NULL;
    httpd_config_t httpd_config = HTTPD_DEFAULT_CONFIG();

    if(httpd_start(&httpd_server, &httpd_config) == ESP_OK){
        httpd_register_uri_handler(httpd_server, &root);
        return httpd_server;
    } else{
        return NULL;
    }
}

static esp_err_t root_get_handler(httpd_req_t *req){
    extern unsigned char view_start[] asm("_binary_view_html_start");
    extern unsigned char view_end[] asm("_binary_view_html_end");
    const size_t view_size = (view_end - view_start);
    char viewHtml[view_size];
    memcpy(viewHtml, view_start, view_size);

    // Only for testing purposes
    srand(time(NULL));
    int random_number = rand() % 100;
    //

    char viewHtmlUpdated[view_size - HTML_NEG_OFFSET];

    sprintf(viewHtmlUpdated, viewHtml, random_number);


    httpd_resp_send(req, (char*) viewHtmlUpdated, view_size - HTML_NEG_OFFSET);

    return ESP_OK;
}
//