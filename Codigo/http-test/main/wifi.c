#include "wifi.h"

// Defines
#define AP_SSID     "Hornito"       //Wi-Fi network name (SSID)
#define AP_PASSWORD "noseashater"   //Wi-Fi network password
//

// Private function prototypes
static void wifi_lwip_init();       //Initialize LwIP network stack and Wi-Fi driver
static void wifi_config();          //Configure Wi-Fi connection in AP mode with specified SSID and password
static void wifi_start();           //Start Wi-Fi connection in AP mode
//

// Functions
void setup_wifi(){
    wifi_lwip_init();
    wifi_config();
    wifi_start();
}
//Initialize LwIP network stack and Wi-Fi driver
static void wifi_lwip_init(){
    nvs_flash_init();                       //Initialize non-volatile flash memory (NVS) for configuration storage
    esp_netif_init();                       //Initialize ESP-IDF network interface
    esp_event_loop_create_default();        //Create a default event loop to handle network events
    esp_netif_create_default_wifi_ap();     //Create a Wi-Fi interface in access point (AP) mode
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT(); //Default Wi-Fi configuration
    wifi_config.nvs_enable = 0;             //Disable NVS usage for Wi-Fi configuration
    esp_wifi_init(&wifi_config);            //Initialize Wi-Fi driver with the provided configuration
}
//Configure Wi-Fi connection in AP mode with specified SSID and password
static void wifi_config(){
    esp_wifi_set_mode(WIFI_MODE_AP);        //Set Wi-Fi mode to access point (AP) mode

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_SSID,                //Set Wi-Fi network name (SSID)
            .password = AP_PASSWORD,        //Set Wi-Fi network password
            .ssid_len = strlen(AP_SSID),    //Calculate SSID length
            .authmode = WIFI_AUTH_WPA2_PSK, //Set authentication mode (WPA2-PSK)
            .max_connection = 10,           //Set maximum number of allowed clients
        }
    };

    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);  //Set AP mode parameters with the provided configuration
}

static void wifi_start(){
    esp_wifi_start();
}
//