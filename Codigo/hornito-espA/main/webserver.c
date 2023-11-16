#include "webserver.h"

// Defines
#define HTML_NEG_OFFSET       1                         // Offset value for HTML manipulation.
#define DATA_ARRAY_SIZE       100                       // Maximum number of data entries.
#define TIME_ARRAY_SIZE       100                       // Maximum length of time string.
#define POWER_ARRAY_SIZE      100                       // Maximum length of power string.
#define TEMP_ARRAY_SIZE       100                       // Maximum length of temperature string.
#define UPDATE_DATA_DELAY_MS  5000                      // Period of updating data task.
#define POWER_QUEUE_DELAY_MS  5000                      // Delay for Queue.

#define DATA_TASK_MONITORING 0
#define TASK_MONITOR_DELAY_MS 2000                      // Delay for Monitor Task.
//

// Typedefs
typedef struct{                                         //Struct to store data: time, power, temperature.
    char tiempo[TIME_ARRAY_SIZE];
    char potencia[POWER_ARRAY_SIZE];
    char temperaturaPF[TEMP_ARRAY_SIZE];
    char temperaturaPC[TEMP_ARRAY_SIZE];
} Measurement;
//

// Global variables
static int data_n = 0;                                  // Number of data entries.
static int update_while = 1;                            // Update data task condition.
static Measurement measurements[DATA_ARRAY_SIZE];       // Array of measurements.
//

// Handles
static TaskHandle_t xTaskUpdateData_handle;             // Task handle for updating data task.
#if DATA_TASK_MONITORING
static TaskHandle_t xTaskUpdateData_Monitoring_handle;  // Task handle for updating data task.
#endif
static httpd_handle_t httpd_server = NULL;              // Handle for HTTP server.
static SemaphoreHandle_t mutexData = NULL;              // Semaphore Handle for mutex.
//

//ESP-LOG TAGS
const static char* TAG_UPDATE = "Update data task";     //TAG for ESP-LOGs.
//

// Private function prototypes
static esp_err_t root_get_handler(httpd_req_t *req);
static esp_err_t time_post_handler(httpd_req_t *req);
static esp_err_t get_data_handler(httpd_req_t *req);
static esp_err_t save_shutodwn_handler(httpd_req_t *req);
static void vTaskUpdateData();
#if DATA_TASK_MONITORING
static void vTaskUpdateData_Monitoring();
#endif
//

// URI handlers
static const httpd_uri_t root = {
    .uri = "/",                         //Root URI (Uniform Resource Identifier) path.
    .method = HTTP_GET,                 //HTTP GET method.
    .handler = root_get_handler,        //Handler function for root URI GET requests.
};

static const httpd_uri_t time_post = {
    .uri = "/post_time",                //URI path for handling time POST requests.
    .method = HTTP_POST,                //HTTP POST method.
    .handler = time_post_handler,       //Handler function for time POST requests.
};

static const httpd_uri_t get_data = {
    .uri = "/get_data",                 //URI path for handling data retrieval requests.
    .method = HTTP_GET,                 //HTTP GET method.
    .handler = get_data_handler,        //Handler function for data retrieval requests.
};

static const httpd_uri_t save_shutodwn = {
    .uri = "/saveShutdown",             //URI path for handling data retrieval requests.
    .method = HTTP_GET,                 //HTTP GET method.
    .handler = save_shutodwn_handler,   //Handler function for data retrieval requests.
};
//

// Functions
// Create the data update task and mutex to access data struct.
static void create_data_tasks(){ 
    if(mutexData == NULL){
        mutexData = xSemaphoreCreateMutex();
    }

    xTaskCreatePinnedToCore(vTaskUpdateData,
                            "vTaskUpdateData",
                            configMINIMAL_STACK_SIZE * 5,
                            NULL,
                            tskIDLE_PRIORITY + 4,
                            &xTaskUpdateData_handle,
                            1);
   
    #if DATA_TASK_MONITORING
    xTaskCreatePinnedToCore(vTaskUpdateData_Monitoring,
                           "vTaskUpdateData Monitoring",
                            configMINIMAL_STACK_SIZE * 5,
                            NULL,
                            tskIDLE_PRIORITY + 1,
                            &xTaskUpdateData_Monitoring_handle,
                            0);
    #endif

}

// Task function for updating data struct periodically.
static void vTaskUpdateData(){
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while(update_while == 1){
        xSemaphoreGive(xSemaphorePower);                                                                    
        xSemaphoreGive(xSemaphoreControlSystem);
        xSemaphoreGive(xSemaphorePeltier);

        float powerValue = 0.0;
        float hotTempValue = 0.0;
        float coldTempValue = 0.0;
        
        if(xQueueReceive( xQueuePower , &powerValue,  pdMS_TO_TICKS(POWER_QUEUE_DELAY_MS))){                // Check and process queue data.
            if (xSemaphoreTake(mutexData, portMAX_DELAY)) {
                sprintf(measurements[data_n].potencia, "%.2f", powerValue);                                 // Convert value to a string and store it in data struct.
                xSemaphoreGive(mutexData);
            }
        }else{
            ESP_LOGE(TAG_UPDATE, "Power Queue timeout");
        }
        
        if(xQueueReceive( xQueueControlSystem , &hotTempValue,  pdMS_TO_TICKS(POWER_QUEUE_DELAY_MS))){
            if(xSemaphoreTake(mutexData, portMAX_DELAY)){
                sprintf(measurements[data_n].temperaturaPC, "%.2f", hotTempValue);
                xSemaphoreGive(mutexData);
            }
        }else{
            ESP_LOGE(TAG_UPDATE, "Control System Queue timeout");
        }

        /*DEBUG - BORRAR E IMPLEMENTAR QUEUE DE PELTIER

        if (xSemaphoreTake(mutexData, portMAX_DELAY)) {
            strncpy(measurements[data_n].temperaturaPF, "80.00", TEMP_ARRAY_SIZE);
            xSemaphoreGive(mutexData);
        }

             */

        if(xQueueReceive( xQueuePeltier , &coldTempValue,  pdMS_TO_TICKS(POWER_QUEUE_DELAY_MS))){
            sprintf(measurements[data_n].temperaturaPF, "%.2f", coldTempValue);
        }else{
            ESP_LOGE(TAG_UPDATE, "Peltier Queue timeout");
        }

        strncpy(measurements[data_n].tiempo, get_time(), TIME_ARRAY_SIZE); 

        data_n++;                                                                                           //Increment data entries counter.
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(UPDATE_DATA_DELAY_MS));
    }
    vTaskDelete(NULL);                                                                                      // Self-delete the task when the loop exits.
}

#if DATA_TASK_MONITORING
// Task function for monitoring the stack usage of the 'xTaskUpdateData' task.
static void vTaskUpdateData_Monitoring(){
    while(true){
        ESP_LOGW(TAG_UPDATE, "Task Update Data: %u bytes", uxTaskGetStackHighWaterMark(xTaskUpdateData_handle));
        vTaskDelay(pdMS_TO_TICKS(TASK_MONITOR_DELAY_MS));           
    }
}
#endif

// Starts  webserver and registers URI handlers.
void start_webserver(){
    httpd_config_t httpd_config = HTTPD_DEFAULT_CONFIG();             // Default HTTP server configuration.
    httpd_config.stack_size = 5*4096;                                 // Stack size for server tasks.
    //httpd_config.task_priority = tskIDLE_PRIORITY + 2;
    //httpd_config.core_id = 1;

    if(httpd_start(&httpd_server, &httpd_config) == ESP_OK){
        httpd_register_uri_handler(httpd_server, &root);              // Register handler for root URI.
        httpd_register_uri_handler(httpd_server, &time_post);         // Register handler for time POST requests.
        httpd_register_uri_handler(httpd_server, &get_data);          // Register handler for data retrieval requests.
        httpd_register_uri_handler(httpd_server, &save_shutodwn);     // Register handler for saving and stopping requests.
    }
}

// Handles GET requests to the root URI.
static esp_err_t root_get_handler(httpd_req_t *req){
    extern unsigned char view_start[] asm("_binary_view_html_start"); // Access binary data of HTML template.
    extern unsigned char view_end[] asm("_binary_view_html_end");     // Access end of binary data of HTML template.
    const size_t view_size = (view_end - view_start);                 // Calculate HTML template size.
    char viewHtml[view_size];
    memcpy(viewHtml, view_start, view_size);                          // Copy HTML template to a buffer.
    httpd_resp_send(req, (char*) viewHtml, view_size);                // Send the current HTML template as response.
    return ESP_OK;
}

// Handles POST requests to "/post_time" URI.
esp_err_t time_post_handler(httpd_req_t *req)
{
    char buf[100];                                                    // Buffer to store POST data.
    int ret, remaining = req->content_len;                            // Remaining bytes to read.
    update_while = 1;
    create_tasks();

    while (remaining > 0) {                                           // Read POST data sent from the form.
        if ((ret = httpd_req_recv(req, buf, sizeof(buf))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);                             // Handle timeout error.
            }
            return ESP_FAIL;
        }

        for(int i = 0; i < ret; i++){
            printf("%c", buf[i]);
        }
        printf("\n");
        
        char *ptr;
        int segundos = strtol(buf + 21, &ptr, 10);                    // Extract seconds data from POST request.

        set_time(segundos);                         

        remaining -= ret;                                             // Update remaining bytes count.
    }

    create_data_tasks();

    httpd_resp_set_status(req, "303 See Other");                      //Send a 303 See Other redirect response.
    httpd_resp_set_hdr(req, "Location", "/");                         //Redirect to root URI.
    httpd_resp_send(req, NULL, 0);                                    //Send redirect response.

    return ESP_OK;
}

// Handles GET requests to "/get_data" URI.
esp_err_t get_data_handler(httpd_req_t *req)
{
    cJSON *datos = cJSON_CreateArray();                                                     // Create a JSON array to store measurements data.
    for(int i = 0; i < data_n; i++){
        cJSON *dato = cJSON_CreateObject();                                                 // Create a JSON object for each measurement entry.
        if (xSemaphoreTake(mutexData, portMAX_DELAY)) {
            cJSON_AddStringToObject(dato, "tiempo", measurements[i].tiempo);                // Add "tiempo" field to JSON object.
            cJSON_AddStringToObject(dato, "potencia", measurements[i].potencia);            // Add "potencia" field to JSON object.
            cJSON_AddStringToObject(dato, "temperaturaPF", measurements[i].temperaturaPF);  // Add "temperaturaPF" field to JSON object.
            cJSON_AddStringToObject(dato, "temperaturaPC", measurements[i].temperaturaPC);  // Add "temperaturaPC" field to JSON object.        
            xSemaphoreGive(mutexData);
        }
        cJSON_AddItemToArray(datos, dato);                                                  // Add the JSON object to the JSON array.
    }

    char *json_str = cJSON_PrintUnformatted(datos);                                         // Generate a JSON string from the data.

    httpd_resp_set_type(req, "application/json");                                           // Configure HTTP response with JSON data.
    httpd_resp_send(req, json_str, strlen(json_str));
    
    cJSON_Delete(datos);                                                                    // Clean up memory.
    free(json_str);

    return ESP_OK;
}

// Handles GET requests to "/saveShutdown" URI.
static esp_err_t save_shutodwn_handler(httpd_req_t *req){

    update_while = 0;                                                                      // To stop Update Data Task while loop.

    static char buffer[3072];                                                              // Create a buffer for file data.
    buffer[0] = '\0';                                                                      // Initialize the buffer as an empty string.

    snprintf(buffer, sizeof(buffer), "Fecha y Hora\tPotencia [W]\tTemperatura PF [°C]\tTemperatura PC [°C]\n"); //Build the header line.
        
    for (int i = 0; i < data_n; i++) {                                                     //Build the content of the text file in memory.
        snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer),                 //Append data to the buffer
                 "%s\t%s\t%s\t%s\n",
                 measurements[i].tiempo, measurements[i].potencia,
                 measurements[i].temperaturaPF, measurements[i].temperaturaPC);
    }

    httpd_resp_set_type(req, "text/plain");                                                 //Configure the HTTP response with the in-memory text file.
    httpd_resp_send(req, buffer, strlen(buffer));

    delete_tasks();

    return ESP_OK;
}