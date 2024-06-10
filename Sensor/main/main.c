
#include <string.h>
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "driver/adc.h"
#include "esp_log.h"

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

#define MEASUREMENTS_FOR_AVERAGE 5
int adc_raw[MEASUREMENTS_FOR_AVERAGE];
#define PAYLOAD_SIZE 4


void send (int humidity) {
    // init wifi & esp_now
    wifi_init();
    espnow_init();
    // init payload
    int i = 0;
    uint8_t payload[PAYLOAD_SIZE];
    for (int y = 0; y<PAYLOAD_SIZE; y++) {
        payload[y] = y;
    }

    // build payload
    payload[0] = 0x00;                                 // Protocol version
    payload[1] = 0x01;
    *(payload + 2) = ((humidity >> 8) & 0xff)%256;  // MSB humidity
    *(payload + 3) = (humidity & 0xff)%256;         // LSB humidity

    // send payload
    esp_now_send(BORADCAST_MAC, payload, PAYLOAD_SIZE);
    // deinit wifi & espnow to be able to read next cycle
    esp_now_deinit();
    esp_wifi_stop();
    esp_wifi_deinit();
}

void app_main(void)
{
    // Initialize NVS required for WIFI
    nvs_flash_init();
    // config ADC2
    adc2_config_channel_atten(ADC2_CHANNEL_3, ADC_ATTEN_DB_11);

    while (1)
    {
        // measure multiple times and take average
        int sum = 0;
        // read adc value
        for (int i = 0; i< MEASUREMENTS_FOR_AVERAGE; i++) {
            esp_err_t reading = adc2_get_raw(ADC2_CHANNEL_4,ADC_WIDTH_BIT_DEFAULT,&adc_raw[i]);
            vTaskDelay(100/portTICK_PERIOD_MS);
            sum += adc_raw[i];
        }
        // send data
        send(sum/MEASUREMENTS_FOR_AVERAGE);
        // set to sleep
        vTaskDelay(5000/portTICK_PERIOD_MS);
    }
    
    // 10 bit -> 2byte feuchtigkeit
    // feuchtigkeit: ad -> wandler 0-3.3V 0 -> 1024 -> 2byte (1024 = 0x0400)
    // batteriespannung: ad -> wandler 0-5V 0 -> 1024 -> 2byte (1024 = 0x0400)
    // 250 

}