#include "webserver.h"
#include "cJSON.h"

#define HTML_NEG_OFFSET 1

// Private function prototypes
static esp_err_t root_get_handler(httpd_req_t *req);
static esp_err_t time_post_handler(httpd_req_t *req);
static esp_err_t obtener_datos_handler(httpd_req_t *req);
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

static const httpd_uri_t obtener_datos = {
    .uri = "/obtener_datos",
    .method = HTTP_GET,
    .handler = obtener_datos_handler,
};
//

// Functions
httpd_handle_t start_webserver(){
    httpd_handle_t httpd_server = NULL;
    httpd_config_t httpd_config = HTTPD_DEFAULT_CONFIG();
    httpd_config.stack_size = 2*4096;

    if(httpd_start(&httpd_server, &httpd_config) == ESP_OK){
        httpd_register_uri_handler(httpd_server, &root);
        httpd_register_uri_handler(httpd_server, &time_post);
        httpd_register_uri_handler(httpd_server, &obtener_datos);
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

    // Después de procesar los datos, envía una respuesta de redirección 303 (See Other)
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

esp_err_t obtener_datos_handler(httpd_req_t *req)
{
    // Simula datos de ejemplo (puedes cargar estos datos desde tu fuente real)
    cJSON *datos = cJSON_CreateArray();
    cJSON *dato1 = cJSON_CreateObject();
    cJSON_AddStringToObject(dato1, "tiempo", "12:30:00");
    cJSON_AddStringToObject(dato1, "potencia", "120");
    cJSON_AddStringToObject(dato1, "temperaturaPF", "12°C");
    cJSON_AddStringToObject(dato1, "temperaturaPC", "58°C");
    cJSON_AddItemToArray(datos, dato1);

    cJSON *dato2 = cJSON_CreateObject();
    cJSON_AddStringToObject(dato2, "tiempo", "12:35:00");
    cJSON_AddStringToObject(dato2, "potencia", "130");
    cJSON_AddStringToObject(dato2, "temperaturaPF", "13°C");
    cJSON_AddStringToObject(dato2, "temperaturaPC", "59°C");
    cJSON_AddItemToArray(datos, dato2);

    // Genera una cadena JSON a partir de los datos
    char *json_str = cJSON_PrintUnformatted(datos);

    // Configura la respuesta HTTP con los datos JSON
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));

    // Limpia la memoria
    cJSON_Delete(datos);
    free(json_str);

    return ESP_OK;
}

//