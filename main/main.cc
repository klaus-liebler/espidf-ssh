#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"

#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h>
#include "driver/spi_master.h"
#include <ethernet.hh>
#define TAG "WIFI"


static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1


/*
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		ESP_LOGI(TAG, "Started! Now connecting");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
	else{
		//ESP_LOGI(TAG, "Unknkown event %i - %ld ", event_base, event_id);
	}
}

void wifi_init_sta(const char* ssid, const char* pass)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    
    wifi_config_t wifi_config = {};
    strlcpy((char*) wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strlcpy((char*) wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
	ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);


    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap");
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to ap");
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}
*/
static void
initialize_nvs()
{
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK( nvs_flash_erase() );
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);
}

extern "C" void start_sshd(void);

extern "C" void
app_main(void)
{
	//initialize_nvs();
	/* replace with SSID and passphrase */
	//wifi_init_sta(CONFIG_NETWORK_WIFI_STA_SSID, CONFIG_NETWORK_WIFI_STA_PASSWORD);
    WIFI_ETH::initETH(false, SPI2_HOST, GPIO_NUM_13, GPIO_NUM_11, GPIO_NUM_12, SPI_MASTER_FREQ_20M, GPIO_NUM_21, GPIO_NUM_10, GPIO_NUM_14, 1);
	start_sshd();
}

