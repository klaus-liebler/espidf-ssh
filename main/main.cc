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
#include "sshd.hh"
#include "minicli.hh"
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

const char *hardcoded_example_host_key =
	"-----BEGIN OPENSSH PRIVATE KEY-----\n"
	"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\n"
	"-----END OPENSSH PRIVATE KEY-----\n";


extern "C" void
app_main(void)
{
	//initialize_nvs();
	/* replace with SSID and passphrase */
	//wifi_init_sta(CONFIG_NETWORK_WIFI_STA_SSID, CONFIG_NETWORK_WIFI_STA_PASSWORD);
    WIFI_ETH::initETH(false, SPI2_HOST, GPIO_NUM_13, GPIO_NUM_11, GPIO_NUM_12, SPI_MASTER_FREQ_20M, GPIO_NUM_21, GPIO_NUM_10, GPIO_NUM_14, 1);
	MiniCli *cli = new MiniCli();
	sshd::SshDemon::InitAndRunSshD(hardcoded_example_host_key, cli);
}

