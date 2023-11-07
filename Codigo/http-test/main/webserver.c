#include "webserver.h"

#define HTML_NEG_OFFSET 1   // Offset value for HTML manipulation.
#define DATA_ARRAY_SIZE 100 // Maximum number of data entries.

#define TIME_ARRAY_SIZE 100  // Maximum length of time string.
#define POWER_ARRAY_SIZE 100 // Maximum length of power string.
#define TEMP_ARRAY_SIZE 100  // Maximum length of temperature string.

#define UPDATE_DATA_DELAY_MS 5000  //Defines the period of updating data task

// Typedefs
typedef struct{
    char tiempo[TIME_ARRAY_SIZE];
    char potencia[POWER_ARRAY_SIZE];
    char temperaturaPF[TEMP_ARRAY_SIZE];
    char temperaturaPC[TEMP_ARRAY_SIZE];
} Measurement;
//

// Handles
static TaskHandle_t xTaskUpdateData_handle;

// Private function prototypes
static esp_err_t root_get_handler(httpd_req_t *req);        //Handler for GET requests to the root URI.
static esp_err_t time_post_handler(httpd_req_t *req);       //Handler for POST requests to "/post_time" URI.
static esp_err_t get_data_handler(httpd_req_t *req);   //Handler for GET requests to "/get_data" URI.
static void vTaskUpdateData();
//

// Global variables
static int data_n = 0;                              //Number of data entries.
static Measurement measurements[DATA_ARRAY_SIZE];   //Array of measurements.
//

// URI handlers
static const httpd_uri_t root = {
    .uri = "/",                         //Root URI (Uniform Resource Identifier) path
    .method = HTTP_GET,                 //HTTP GET method
    .handler = root_get_handler,        //Handler function for root URI GET requests
};

static const httpd_uri_t time_post = {
    .uri = "/post_time",                //URI path for handling time POST requests
    .method = HTTP_POST,                //HTTP POST method
    .handler = time_post_handler,       //Handler function for time POST requests
};

static const httpd_uri_t get_data = {
    .uri = "/get_data",            //URI path for handling data retrieval requests
    .method = HTTP_GET,                 //HTTP GET method
    .handler = get_data_handler,   //Handler function for data retrieval requests
};
//

// Functions
static void create_data_tasks(){
    xTaskCreatePinnedToCore(vTaskUpdateData,
                            "vTaskUpdateData",
                            configMINIMAL_STACK_SIZE * 5,
                            NULL,
                            tskIDLE_PRIORITY + 2,
                            &xTaskUpdateData_handle,
                            0);
}

static void vTaskUpdateData(){
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while(true){
        strncpy(measurements[data_n].tiempo, get_time(), TIME_ARRAY_SIZE); //Get current time and store it in the measurements array
        strncpy(measurements[data_n].potencia, "100.00 W", POWER_ARRAY_SIZE);
        sprintf(measurements[data_n].temperaturaPF, "%.2f °C", get_ntc_temperature_c(get_adc_voltage_mv_multisampling(ADC_UNIT_1, ADC_CHANNEL_0)));
        strncpy(measurements[data_n].temperaturaPC, "80.00 °C", TEMP_ARRAY_SIZE);
        data_n++;   //Increment data entries counter
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(UPDATE_DATA_DELAY_MS));
    }
}

httpd_handle_t start_webserver(){
    httpd_handle_t httpd_server = NULL;
    httpd_config_t httpd_config = HTTPD_DEFAULT_CONFIG();   //Default HTTP server configuration
    httpd_config.stack_size = 2*4096;                       //Double the default stack size for server tasks

    if(httpd_start(&httpd_server, &httpd_config) == ESP_OK){
        httpd_register_uri_handler(httpd_server, &root);            //Register handler for root URI
        httpd_register_uri_handler(httpd_server, &time_post);       //Register handler for time POST requests
        httpd_register_uri_handler(httpd_server, &get_data);   //Register handler for data retrieval requests
        return httpd_server;                                        //Return HTTP server handle
    } else{
        return NULL;
    }
}

//Handles GET requests to the root URI
static esp_err_t root_get_handler(httpd_req_t *req){
    extern unsigned char view_start[] asm("_binary_view_html_start");   //Access binary data of HTML template
    extern unsigned char view_end[] asm("_binary_view_html_end");       //Access end of binary data of HTML template
    const size_t view_size = (view_end - view_start);                   //Calculate HTML template size
    char viewHtml[view_size];
    memcpy(viewHtml, view_start, view_size);                            //Copy HTML template to a buffer

    // Only for testing purposes
    //srand(time(NULL));
    //int random_number = rand() % 100;
    //

    //char viewHtmlUpdated[view_size - HTML_NEG_OFFSET];

    //sprintf(viewHtmlUpdated, viewHtml, random_number);

    //httpd_resp_send(req, (char*) viewHtmlUpdated, view_size - HTML_NEG_OFFSET);

    httpd_resp_send(req, (char*) viewHtml, view_size);                  //Send the current HTML template as response
    return ESP_OK;
}

// Handles POST requests to "/post_time" URI
esp_err_t time_post_handler(httpd_req_t *req)
{
    char buf[100];                                                  //Buffer to store POST data
    int ret, remaining = req->content_len;                          //Remaining bytes to read
    
    printf("Content length: %d\n", req->content_len);

    //Read POST data sent from the form
    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, sizeof(buf))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                // Handle timeout error
                httpd_resp_send_408(req);
            }
            return ESP_FAIL;
        }
        // Print received POST data
        for(int i = 0; i < ret; i++){
            printf("%c", buf[i]);
        }
        printf("\n");
        
        char *ptr;
        int segundos = strtol(buf + 21, &ptr, 10);  //Extract seconds data from POST request
        set_time(segundos);                         //Process the received data, for example, store the current time
        // Procesar los datos recibidos, por ejemplo, guardar la hora actual en una variable
        // buf contiene los datos enviados desde el formulario
        // Puedes extraer y procesar la hora actual aquí
        remaining -= ret;                           // Update remaining bytes count
    }
    create_data_tasks();
    //After processing data, send a 303 See Other redirect response
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");    //Redirect to root URI
    httpd_resp_send(req, NULL, 0);               //Send redirect response

    

    return ESP_OK;
}
//Handles GET requests to "/get_data" URI
esp_err_t get_data_handler(httpd_req_t *req)
{
    cJSON *datos = cJSON_CreateArray();
    for(int i = 0; i < data_n; i++){
        cJSON *dato = cJSON_CreateObject();
        cJSON_AddStringToObject(dato, "tiempo", measurements[i].tiempo);
        cJSON_AddStringToObject(dato, "potencia", measurements[i].potencia);
        cJSON_AddStringToObject(dato, "temperaturaPF", measurements[i].temperaturaPF);
        cJSON_AddStringToObject(dato, "temperaturaPC", measurements[i].temperaturaPC);
        cJSON_AddItemToArray(datos, dato);
    }

    //Generate a JSON string from the data
    char *json_str = cJSON_PrintUnformatted(datos);

    //Configure HTTP response with JSON data
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));

    //Clean up memory
    cJSON_Delete(datos);
    free(json_str);

    return ESP_OK;
}

//