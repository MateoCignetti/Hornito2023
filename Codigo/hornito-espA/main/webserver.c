#include "webserver.h"

// Defines
#define HTML_NEG_OFFSET       1      // Offset value for HTML manipulation.
#define DATA_ARRAY_SIZE       100    // Maximum number of data entries.
#define TIME_ARRAY_SIZE       100    // Maximum length of time string.
#define POWER_ARRAY_SIZE      100    // Maximum length of power string.
#define TEMP_ARRAY_SIZE       100    // Maximum length of temperature string.
#define UPDATE_DATA_DELAY_MS  5000   // Period of updating data task.
#define POWER_QUEUE_DELAY_MS  5000   // Delay for Queue
#define TASK_MONITOR_DELAY_MS 2000  
//

// Typedefs
typedef struct{
    char tiempo[TIME_ARRAY_SIZE];
    char potencia[POWER_ARRAY_SIZE];
    char temperaturaPF[TEMP_ARRAY_SIZE];
    char temperaturaPC[TEMP_ARRAY_SIZE];
} Measurement;
//

// Global variables
static int data_n = 0;                              //Number of data entries.
static int update_while = 1;                        //Update data task condition.
static Measurement measurements[DATA_ARRAY_SIZE];   //Array of measurements.
//

// Handles
static TaskHandle_t xTaskUpdateData_handle;             //Task handle for updating data task.
static TaskHandle_t xTaskUpdateData_Monitoring_handle;  //Task handle for updating data task.
static httpd_handle_t httpd_server = NULL;              //Initialize URI handlers config struct as NULL.
static SemaphoreHandle_t mutexData = NULL;
//

//ESP-LOG TAGS
const static char* TAG_UPDATE = "UPDATE DATA TASK";
//

// Private function prototypes
static esp_err_t root_get_handler(httpd_req_t *req);        //Handler for GET requests to the root URI.
static esp_err_t time_post_handler(httpd_req_t *req);       //Handler for POST requests to "/post_time" URI.
static esp_err_t get_data_handler(httpd_req_t *req);        //Handler for GET requests to "/get_data" URI.
static esp_err_t save_shutodwn_handler(httpd_req_t *req);   //Handler for POST requests to "/saveShutdown" URI.
static void vTaskUpdateData();
static void xTaskUpdateData_Monitoring();
static void create_mutex_data();
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
//Create the data update task.
static void create_data_tasks(){ 

    xTaskCreatePinnedToCore(vTaskUpdateData,
                            "vTaskUpdateData",
                            configMINIMAL_STACK_SIZE * 3,
                            NULL,
                            tskIDLE_PRIORITY + 1,
                            &xTaskUpdateData_handle,
                            1);
   

    xTaskCreatePinnedToCore(xTaskUpdateData_Monitoring,
                           "vTaskUpdateData Monitoring",
                            configMINIMAL_STACK_SIZE * 2,
                            NULL,
                            tskIDLE_PRIORITY + 1,
                            &xTaskUpdateData_Monitoring_handle,
                            0);

    create_mutex_data();
}

void create_mutex_data(){
    if (mutexData == NULL) {
        mutexData = xSemaphoreCreateMutex();
    }
}
/*/Task function for updating data periodically.
static void vTaskUpdateData(){
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while(update_while == 1){
        xSemaphoreGive(xSemaphorePower);
        float powerValue = 0.0;
        if(xQueueReceive( xQueuePower , &powerValue,  pdMS_TO_TICKS(POWER_QUEUE_DELAY_MS))){
            //Update measurements with current data.
            strncpy(measurements[data_n].tiempo, get_time(), TIME_ARRAY_SIZE); 
            sprintf(measurements[data_n].potencia, "%.2f", powerValue);
            sprintf(measurements[data_n].temperaturaPF, "%.2f", get_ntc_temperature_c(get_adc_voltage_mv_multisampling(ADC_UNIT_1, ADC_CHANNEL_0)));
            strncpy(measurements[data_n].temperaturaPC, "80.00", TEMP_ARRAY_SIZE);
            data_n++;   //Increment data entries counter.
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(UPDATE_DATA_DELAY_MS));
        }else{
            ESP_LOGE("vTaskUpdateData", "Queue timeout");
        }
    }
    vTaskDelete(NULL);  //Self delete.
}*/


//Task function for updating data periodically.
static void vTaskUpdateData(){
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while(update_while == 1){
        xSemaphoreGive(xSemaphorePower);
        xSemaphoreGive(xSemaphoreControlSystem);
        //xSemaphoreGive(xSemaphorePeltier);

        float powerValue = 0.0;
        float hotTempValue = 0.0;
        //float coldTempValue = 0.0;
        
        if(xQueueReceive( xQueuePower , &powerValue,  pdMS_TO_TICKS(POWER_QUEUE_DELAY_MS))){
            if (xSemaphoreTake(mutexData, portMAX_DELAY)) {
                sprintf(measurements[data_n].potencia, "%.2f", powerValue);
                xSemaphoreGive(mutexData);
            }
        }else{
            ESP_LOGE("vTaskUpdateData", "Power Queue timeout");
        }
        
        if(xQueueReceive( xQueueControlSystem , &hotTempValue,  pdMS_TO_TICKS(POWER_QUEUE_DELAY_MS))){
            if(xSemaphoreTake(mutexData, portMAX_DELAY)){
                sprintf(measurements[data_n].temperaturaPC, "%.2f", hotTempValue);
                xSemaphoreGive(mutexData);
            }
        }else{
            ESP_LOGE("vTaskUpdateData", "Control System Queue timeout");
        }

        /*DEBUG*/

        if (xSemaphoreTake(mutexData, portMAX_DELAY)) {
            strncpy(measurements[data_n].temperaturaPF, "80.00", TEMP_ARRAY_SIZE);
            xSemaphoreGive(mutexData);
        }

        /*if(xQueueReceive( xQueuePeltier , &coldTempValue,  pdMS_TO_TICKS(POWER_QUEUE_DELAY_MS))){
            sprintf(measurements[data_n].temperaturaPF, "%.2f", coldTempValue);
        }else{
            ESP_LOGE("vTaskUpdateData", "Peltier Queue timeout");
        }*/

        strncpy(measurements[data_n].tiempo, get_time(), TIME_ARRAY_SIZE); 

        data_n++;   //Increment data entries counter.
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(UPDATE_DATA_DELAY_MS));
    }
    vTaskDelete(NULL);  //Self delete.
}

static void xTaskUpdateData_Monitoring(){
    while(true){
        
        ESP_LOGW(TAG_UPDATE, "Task Update Data: %u bytes", uxTaskGetStackHighWaterMark(xTaskUpdateData_handle));
        vTaskDelay(pdMS_TO_TICKS(TASK_MONITOR_DELAY_MS));   
        
        //if(xTaskUpdateData_handle != NULL){ 
        //UBaseType_t taskUpdateData_memory = uxTaskGetStackHighWaterMark(xTaskUpdateData_handle);
        //ESP_LOGW(TAG_UPDATE, "Task Update Data: %u bytes", taskUpdateData_memory);
        //    if(taskUpdateData_memory == 0){
        //        xTaskUpdateData_handle = NULL;
        //    }
        //vTaskDelay(pdMS_TO_TICKS(TASK_MONITOR_DELAY_MS));
        //}
        
    }
}


//Starts the webserver and registers URI handlers.
void start_webserver(){
    httpd_config_t httpd_config = HTTPD_DEFAULT_CONFIG();               //Default HTTP server configuration.
    httpd_config.stack_size = 5*4096;                                   //Stack size for server tasks.
    //httpd_config.task_priority = tskIDLE_PRIORITY + 2;
    //httpd_config.core_id = 1;

    if(httpd_start(&httpd_server, &httpd_config) == ESP_OK){
        httpd_register_uri_handler(httpd_server, &root);                //Register handler for root URI.
        httpd_register_uri_handler(httpd_server, &time_post);           //Register handler for time POST requests.
        httpd_register_uri_handler(httpd_server, &get_data);            //Register handler for data retrieval requests.
        httpd_register_uri_handler(httpd_server, &save_shutodwn);       //Register handler for saving and stopping requests.
    }
}

//Handles GET requests to the root URI.
static esp_err_t root_get_handler(httpd_req_t *req){
    extern unsigned char view_start[] asm("_binary_view_html_start");   //Access binary data of HTML template.
    extern unsigned char view_end[] asm("_binary_view_html_end");       //Access end of binary data of HTML template.
    const size_t view_size = (view_end - view_start);                   //Calculate HTML template size.
    char viewHtml[view_size];
    memcpy(viewHtml, view_start, view_size);                            //Copy HTML template to a buffer.
    httpd_resp_send(req, (char*) viewHtml, view_size);                  //Send the current HTML template as response.
    return ESP_OK;
}

// Handles POST requests to "/post_time" URI.
esp_err_t time_post_handler(httpd_req_t *req)
{
    char buf[100];                                                      //Buffer to store POST data.
    int ret, remaining = req->content_len;                              //Remaining bytes to read.
    
    //printf("Content length: %d\n", req->content_len);
    create_tasks();
    //Read POST data sent from the form.
    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, sizeof(buf))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                // Handle timeout error.
                httpd_resp_send_408(req);
            }
            return ESP_FAIL;
        }
        // Print received POST data.
        for(int i = 0; i < ret; i++){
            printf("%c", buf[i]);
        }
        printf("\n");
        
        char *ptr;
        int segundos = strtol(buf + 21, &ptr, 10);  //Extract seconds data from POST request.

        set_time(segundos);                         //Process the received data, for example, store the current time.

        remaining -= ret;                           // Update remaining bytes count.
    }
    create_data_tasks();
    //After processing data, send a 303 See Other redirect response.
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");       //Redirect to root URI.
    httpd_resp_send(req, NULL, 0);                  //Send redirect response.

    return ESP_OK;
}

//Handles GET requests to "/get_data" URI.
esp_err_t get_data_handler(httpd_req_t *req)
{
    cJSON *datos = cJSON_CreateArray();                                                     //Create a JSON array to store measurement data.
    for(int i = 0; i < data_n; i++){
        cJSON *dato = cJSON_CreateObject();                                                 //Create a JSON object for each measurement entry.
        if (xSemaphoreTake(mutexData, portMAX_DELAY)) {
            cJSON_AddStringToObject(dato, "tiempo", measurements[i].tiempo);                //Add "tiempo" field to JSON object.
            cJSON_AddStringToObject(dato, "potencia", measurements[i].potencia);            //Add "potencia" field to JSON object.
            cJSON_AddStringToObject(dato, "temperaturaPF", measurements[i].temperaturaPF);  //Add "temperaturaPF" field to JSON object.
            cJSON_AddStringToObject(dato, "temperaturaPC", measurements[i].temperaturaPC);  //Add "temperaturaPC" field to JSON object.        
            xSemaphoreGive(mutexData);
        }
        cJSON_AddItemToArray(datos, dato);                                                  // Add the JSON object to the JSON array.
    }

    char *json_str = cJSON_PrintUnformatted(datos);  //Generate a JSON string from the data.

    //Configure HTTP response with JSON data.
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));

    //Clean up memory.
    cJSON_Delete(datos);
    free(json_str);

    return ESP_OK;
}

// Handler for GET requests to "/saveShutdown" URI.
static esp_err_t save_shutodwn_handler(httpd_req_t *req){

    delete_tasks();
    update_while = 0;   //Stops Update Data Task while loop.

    static char buffer[3072];  //Create a buffer for file data.
    buffer[0] = '\0';   //Initialize the buffer as an empty string.

    snprintf(buffer, sizeof(buffer), "Fecha y Hora\tPotencia [W]\tTemperatura PF [°C]\tTemperatura PC [°C]\n"); //Build the header line.

    //Build the content of the text file in memory.
    for (int i = 0; i < data_n; i++) {
        //Append data to the buffer
        snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer),
                 "%s\t%s\t%s\t%s\n",
                 measurements[i].tiempo, measurements[i].potencia,
                 measurements[i].temperaturaPF, measurements[i].temperaturaPC);
    }

    //Configure the HTTP response with the in-memory text file.
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, buffer, strlen(buffer));

    return ESP_OK;
}