#include "main.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>


//#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"

#include "esp_wifi.h"

#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "wifi";

static EventGroupHandle_t s_wifi_event_group;

static int s_retry_num = 0;

/* WIFI CONFIGURATION-------------------------------------------*/

#define DEFAULT_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define DEFAULT_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

/* WIFI SoftAP CONFIGURATION-------------------------------------------*/

#define EXAMPLE_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1


static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);
void wifi_init_sta(void);

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data);
void wifi_init_softap(void);

// WIFI Mode

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    //ESP_ERROR_CHECK(esp_event_loop_create_default());
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

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = DEFAULT_ESP_WIFI_SSID,
            .password = DEFAULT_ESP_WIFI_PASS,
             /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	        .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
	        .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };

	//nvs_handle handle;
	esp_err_t err;

    ESP_LOGI(TAG,"Opening Non-Volatile Storage (NVS) handle... ");
    nvs_handle_t my_handle;
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG,"Error (%s) opening NVS handle!", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG,"Done");

		/* allocate buffer */
		//size_t sz = sizeof(wifi_config);
		//uint8_t *buff = (uint8_t*)malloc(sizeof(uint8_t) * sz);
		//memset(buff, 0x00, sizeof(sz));        

        // Read
        ESP_LOGI(TAG,"Reading Wifi SSID from NVS ... ");
        //char NVS_WIFI_SSID[];
        size_t required_size;
        nvs_get_blob(my_handle, "wifi_ssid", NULL, &required_size);
        uint8_t *buff = (uint8_t*)malloc(sizeof(uint8_t) * required_size);
        //uint8_t *buff;
		//memset(buff, 0x00, sizeof(required_size));
        //sz = sizeof(32);
        //uint8_t *nvs_wifi_ssid = malloc(required_size);
        //nvs_wifi_ssid = malloc(required_size);
        err = nvs_get_blob(my_handle, "wifi_ssid", buff, &required_size);
        switch (err) {
            case ESP_OK:
                ESP_LOGI(TAG,"The value of Wifi SSID from NVS is %s", buff);
                //NVS_WIFI_SSID[32] = nvs_wifi_ssid;
                memcpy(wifi_config.sta.ssid, buff, required_size);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                ESP_LOGI(TAG,"The value is not initialized yet!");
                break;
            default :
                ESP_LOGI(TAG,"Error (%s) reading!\n", esp_err_to_name(err));
        }

        //size_t required_size;
        //nvs_get_str(my_handle, "server_name", NULL, &required_size);
        //char* server_name = malloc(required_size);
        //nvs_get_str(my_handle, "server_name", server_name, &required_size);

    //char ESP_WIFI_PASS[] = DEFAULT_ESP_WIFI_PASS;
        ESP_LOGI(TAG,"Reading Wifi Password from NVS ... ");
        //sz = sizeof(32);
        nvs_get_blob(my_handle, "wifi_password", NULL, &required_size);
        //*buff = malloc(required_size);
        buff = (uint8_t*)malloc(sizeof(uint8_t) * required_size);
		//memset(buff, 0x00, sizeof(required_size));
        //uint8_t *nvs_wifi_password = malloc(required_size);
        //nvs_wifi_password = malloc(required_size);
        err = nvs_get_blob(my_handle, "wifi_password", buff, &required_size);
        switch (err) {
            case ESP_OK:
                ESP_LOGI(TAG,"The value of Wifi password from NVS is %s", buff);
                //NVS_WIFI_PASS[64] = nvs_wifi_password;
                memcpy(wifi_config.sta.password, buff, required_size);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                ESP_LOGI(TAG,"The value is not initialized yet!");
                break;
            default :
                ESP_LOGI(TAG,"Error (%s) reading!", esp_err_to_name(err));
        }

        //printf("Reading restart counter from NVS ... ");
        //int32_t restart_counter = 0; // value will default to 0, if not set yet in NVS
        //err = nvs_get_i32(my_handle, "restart_counter", &restart_counter);
        //switch (err) {
        //    case ESP_OK:
        //        printf("Done\n");
        //        printf("Restart counter = %d\n", restart_counter);
        //        break;
        //    case ESP_ERR_NVS_NOT_FOUND:
        //        printf("The value is not initialized yet!\n");
        //        break;
        //    default :
        //        printf("Error (%s) reading!\n", esp_err_to_name(err));
        //}
        nvs_close(my_handle);
    }



    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    
    if (bits & WIFI_CONNECTED_BIT) {
       ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 wifi_config.sta.ssid, wifi_config.sta.password);
        isWifiConnected = true;
        isNetworkConnected = true;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 wifi_config.sta.ssid, wifi_config.sta.password);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}

//End WIFI Mode

// WIFI AP Mode


static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}




void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    //ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = DEFAULT_ESP_WIFI_SSID,
            .ssid_len = strlen(DEFAULT_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = DEFAULT_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(DEFAULT_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             DEFAULT_ESP_WIFI_SSID, DEFAULT_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
    isNetworkConnected = true;
}


//End WIFI AP Mode