#include "wifi.h"

// Defines
#define AP_SSID     "Hornito"       //Nombre de la red Wi-Fi (SSID)
#define AP_PASSWORD "noseashater"   //Contraseña de la red Wi-Fi
//

// Private function prototypes
static void wifi_lwip_init();
static void wifi_config();
static void wifi_start();
//

// Functions
void setup_wifi(){
    wifi_lwip_init();
    wifi_config();
    wifi_start();
}
//Inicialización de la pila de red LwIP y el controlador Wi-Fi
static void wifi_lwip_init(){
    nvs_flash_init();                       //Inicializa la memoria flash no volátil (NVS) para el almacenamiento de configuración
    esp_netif_init();                       //Inicializa la interfaz de red ESP-IDF
    esp_event_loop_create_default();        //Crea un bucle de eventos predeterminado para manejar eventos de red
    esp_netif_create_default_wifi_ap();     //Crea una interfaz Wi-Fi en modo punto de acceso (AP)
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT(); //Configuración predeterminada de Wi-Fi
    wifi_config.nvs_enable = 0;             //Deshabilita el uso de NVS para la configuración Wi-Fi
    esp_wifi_init(&wifi_config);            //Inicializa el controlador Wi-Fi con la configuración proporcionada
}
//Configura la conexión Wi-Fi en modo AP con el SSID y la contraseña especificados
static void wifi_config(){
    esp_wifi_set_mode(WIFI_MODE_AP);        //Configura el modo de Wi-Fi como punto de acceso (AP)

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_SSID,                //Establece el nombre de la red Wi-Fi (SSID)
            .password = AP_PASSWORD,        //Establece la contraseña de la red Wi-Fi
            .ssid_len = strlen(AP_SSID),    //Calcula la longitud del SSID
            .authmode = WIFI_AUTH_WPA2_PSK, //Establece el modo de autenticación (WPA2-PSK)
            .max_connection = 10,           //Establece el número máximo de clientes permitidos
        }
    };

    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);  //Configura los parámetros de la red AP con la configuración proporcionada
}

static void wifi_start(){
    esp_wifi_start();
}
//