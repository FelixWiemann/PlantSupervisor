
#include <string.h>
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_now.h"

#define CONFIG_ESPNOW_CHANNEL 1
static uint8_t BORADCAST_MAC[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

/* WiFi should start before using ESPNOW */
static void wifi_init(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK( esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
#endif
}

static esp_err_t espnow_init(void)
{
    /* Initialize ESPNOW and register sending and receiving callback function. */
    esp_now_init();
#if CONFIG_ESPNOW_ENABLE_POWER_SAVE
    esp_now_set_wake_window(CONFIG_ESPNOW_WAKE_WINDOW);
    esp_wifi_connectionless_module_set_wake_interval(CONFIG_ESPNOW_WAKE_INTERVAL);
#endif

    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL) {
        // ESP_LOGE(TAG, "Malloc peer information fail");
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESP_IF_WIFI_STA;
    peer->encrypt = false;
    memcpy(peer->peer_addr, BORADCAST_MAC, ESP_NOW_ETH_ALEN);
    esp_now_add_peer(peer);
    free(peer);
    return ESP_OK;
}

#define SIZE 15
void app_main(void)
{
    int i = 0;
    uint8_t payload[SIZE];
    for (int y = 0; y<SIZE; y++) {
        payload[y] = 0;
    }

    nvs_flash_init();
    wifi_init();
    espnow_init();
    while (1)
    {
        vTaskDelay(1000/portTICK_PERIOD_MS);
        payload[i%SIZE] ++;
        i++;
        esp_now_send(BORADCAST_MAC, payload, SIZE);
        /* code */
    }
    
    // Initialize NVS
    // 10 bit -> 2byte feuchtigkeit
    // feuchtigkeit: ad -> wandler 0-3.3V 0 -> 1024 -> 2byte (1024 = 0x0400)
    // batteriespannung: ad -> wandler 0-5V 0 -> 1024 -> 2byte (1024 = 0x0400)
    // 250 

}