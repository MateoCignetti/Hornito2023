#include "webserver.h"

#define HTML_NEG_OFFSET 1

// Private function prototypes
static esp_err_t root_get_handler(httpd_req_t *req);
static esp_err_t time_post_handler(httpd_req_t *req);
//

// Global variables
static const httpd_uri_t root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler,
};

static const httpd_uri_t time_post = {
    .uri = "/post_time",
    .method = HTTP_POST,
    .handler = time_post_handler,
};
//

// Functions
httpd_handle_t start_webserver(){
    httpd_handle_t httpd_server = NULL;
    httpd_config_t httpd_config = HTTPD_DEFAULT_CONFIG();

    if(httpd_start(&httpd_server, &httpd_config) == ESP_OK){
        httpd_register_uri_handler(httpd_server, &root);
        httpd_register_uri_handler(httpd_server, &time_post);
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
    //srand(time(NULL));
    //int random_number = rand() % 100;
    //

    //char viewHtmlUpdated[view_size - HTML_NEG_OFFSET];

    //sprintf(viewHtmlUpdated, viewHtml, random_number);


   //httpd_resp_send(req, (char*) viewHtmlUpdated, view_size - HTML_NEG_OFFSET);
    httpd_resp_send(req, (char*) viewHtml, view_size);
    return ESP_OK;
}

esp_err_t time_post_handler(httpd_req_t *req)
{
    char buf[100];
    int ret, remaining = req->content_len;
    
    printf("Content length: %d\n", req->content_len);

    // Leer los datos POST enviados desde el formulario
    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, sizeof(buf))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                // Handle timeout error
                httpd_resp_send_408(req);
            }
            return ESP_FAIL;
        }

        for(int i = 0; i < ret; i++){
            printf("%c", buf[i]);
        }
        printf("\n");
        
        char *ptr;
        int segundos = strtol(buf + 21, &ptr, 10);
        set_time(segundos);
        // Procesar los datos recibidos, por ejemplo, guardar la hora actual en una variable
        // buf contiene los datos enviados desde el formulario
        // Puedes extraer y procesar la hora actual aquí
        remaining -= ret;
    }

    // Enviar una respuesta de éxito
    //httpd_resp_send(req, "Datos recibidos con éxito");
    return ESP_OK;
}

//