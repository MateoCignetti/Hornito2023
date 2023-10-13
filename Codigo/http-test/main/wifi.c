#include "wifi.h"

// Defines
#define AP_SSID "Hornito"
#define AP_PASSWORD "noseashater"
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

static void wifi_lwip_init(){
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config.nvs_enable = 0;
    esp_wifi_init(&wifi_config);
}

static void wifi_config(){
    esp_wifi_set_mode(WIFI_MODE_AP);

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_SSID,
            .password = AP_PASSWORD,
            .ssid_len = strlen(AP_SSID),
            .authmode = WIFI_AUTH_WPA2_PSK,
            .max_connection = 10,
        }
    };

    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
}

static void wifi_start(){
    esp_wifi_start();
}
//