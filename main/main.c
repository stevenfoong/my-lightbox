
/**
 ******************************************************************************
 * @Channel Link    :  https://www.youtube.com/user/wardzx1
 * @file    		:  main1.c
 * @author  		:  Ward Almasarani - Useful Electronics
 * @version 		:  v.1.0
 * @date    		:  Aug 20, 2022
 * @brief   		:
 *
 ******************************************************************************/



/* INCLUDES ------------------------------------------------------------------*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* INCLUDES For WIFI----------------------------------------------------------*/
#include "freertos/event_groups.h"
//#include "wifi.h"

/* INCLUDES For WIFI END------------------------------------------------------*/

#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

/* INCLUDES For WIFI--------------------------------------------*/
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "lwip/err.h"
#include "lwip/sys.h"
/* INCLUDES For WIFI END------------------------------------------------------*/

#include "lvgl.h"

#include "main.h"
#include "lvgl_demo_ui.h"

//Storage

#include "esp_vfs.h"
#include "esp_vfs_fat.h"

// Handle of the wear levelling library instance
//static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

// Mount path for the partition
const char *base_path = "/data";

//End Storage

//#include "webserver.h"

#define DEFAULT_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define DEFAULT_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD

/* WIFI CONFIGURATION-------------------------------------------


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

WIFI SoftAP CONFIGURATION-------------------------------------------*/

/*

#define EXAMPLE_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN

*/

/* FreeRTOS event group to signal when we are connected*/
//static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */

/*
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
*/



//static int wifi_failures = 0;
//const char TAG = "wifi station";
static const char *TAG = "MainApp";
bool isWifiConnected = false;
bool isNetworkConnected = false;


//char NVS_WIFI_SSID[32] = NULL;
//char NVS_WIFI_PASS[64] = NULL;

//static int s_retry_num = 0;















/* WIFI CONFIGURATION END-----------------------------------------------------*/

/* PRIVATE STRUCTRES ---------------------------------------------------------*/

/* VARIABLES -----------------------------------------------------------------*/
//static const char* TAG = "example";
//const char TAG = "example";


/* DEFINITIONS ---------------------------------------------------------------*/

/* MACROS --------------------------------------------------------------------*/

/* PRIVATE FUNCTIONS DECLARATION ---------------------------------------------*/


static void example_increase_lvgl_tick			(void *arg);
static bool example_notify_lvgl_flush_ready		(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);
static void example_lvgl_flush_cb				(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);
static void main_creatSysteTasks				(void);
static void lvglTimerTask						(void* param);
/* FUNCTION PROTOTYPES -------------------------------------------------------*/


static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

static void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // copy a buffer's content to a specific area of the display
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

static void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

static void lvglTimerTask(void* param)
{
	while(1)
	{
		// The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
		lv_timer_handler();

		vTaskDelay(10/portTICK_PERIOD_MS);
	}
}

static void main_creatSysteTasks(void)
{

	xTaskCreatePinnedToCore(lvglTimerTask, "lvgl Timer", 10000, NULL, 4, NULL, 1);
}

// Event handler for Wi-Fi events
/*
static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
    {
        case SYSTEM_EVENT_STA_START: // Wi-Fi station mode started
            ESP_LOGI(TAG, "Wi-Fi station started. Connecting to AP...");
            break;
        case SYSTEM_EVENT_STA_GOT_IP: // Successfully connected to Wi-Fi
            {
                char ip_address[16];
                esp_ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip, ip_address, sizeof(ip_address));
                ESP_LOGI(TAG, "Connected to AP. IP address: %s", ip_address);
            }
            wifi_failures = 0; // Reset the failure counter upon successful connection
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED: // Wi-Fi connection lost
            ESP_LOGW(TAG, "Wi-Fi disconnected");

            wifi_failures++;
            ESP_LOGI(TAG, "Connection failures: %d", wifi_failures);

            if (wifi_failures >= EXAMPLE_ESP_MAXIMUM_RETRY) {
                // Start AP mode after reaching the maximum allowed failures
                ESP_LOGI(TAG, "Starting AP mode...");
                wifi_config_t ap_config = {
                    .ap = {
                        .ssid = "MyESPAP",
                        .password = "MyPassword",
                        .max_connection = 4,
                        .authmode = WIFI_AUTH_WPA_WPA2_PSK,
                    },
                };
                ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
                ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
                ESP_ERROR_CHECK(esp_wifi_start());
                ESP_LOGI(TAG, "AP mode started");
            }
            else {
                // Retry connecting to Wi-Fi
                ESP_LOGI(TAG, "Retrying Wi-Fi connection...");
                vTaskDelay(pdMS_TO_TICKS(5000)); // Wait for 5 seconds before retrying
                esp_wifi_connect();
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}
*/


/**
 * @brief Program starts from here
 *
 */
void app_main(void)
{

    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        ESP_LOGI(TAG,"Get flash size failed");
        return;
    }

    ESP_LOGI(TAG, "This is %s chip with %d CPU core(s), WiFi%s%s, silicon revision v%d.%d, %" PRIu32 "MB %s flash",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
           major_rev, 
           minor_rev, 
           flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    //Storage

    mount_stock_storage();

    //End Storage

    // Initialize NVS

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);



    // End Initialize NVS

    // WIFI Section


    


    //unsigned major_rev = chip_info.revision / 100;
    //unsigned minor_rev = chip_info.revision % 100;
    //printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    /*
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }
    */

    //printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
    //       (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    ESP_LOGI(TAG,"Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(TAG, "Connecting WIFI");
    wifi_init_sta();
    //ESP_LOGI(TAG, "Continue from WIFI");

    if (!isWifiConnected) {

        ESP_LOGI(TAG, "Starting Wifi AP Mode");
        wifi_init_softap();
        //ESP_LOGI(TAG, "Continue from AP");

    }

    if (isNetworkConnected) {

        ESP_LOGI(TAG, "Starting Web Server");
        setup_server();
        //start_webserver(void)
    }

    // End WIFI Section

    // Start Up WIFI

    //ESP_ERROR_CHECK(nvs_flash_init());

    // Initialize the event loop and the default event loop
    //ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize Wi-Fi
    //wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    //ESP_ERROR_CHECK(esp_wifi_init(&cfg));


    // Create an event loop to handle Wi-Fi events
    //ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Register the event handler
    //esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    //ESP_ERROR_CHECK(wifi_event_handler(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    //wifi_config_t wifi_config = {
    //    .sta = {
    //        .ssid = "YourWiFiSSID",
    //        .password = "YourWiFiPassword",
    //    },
    //};

    // Initialize and start Wi-Fi in station mode
    //ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    //ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config)); // Use ESP_IF_WIFI_STA
    //ESP_ERROR_CHECK(esp_wifi_start());

    //ESP_LOGI(TAG, "Wi-Fi station mode initialized. Connecting to AP...");

    // End Start Up WIFI




    static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
    static lv_disp_drv_t disp_drv;      // contains callback functions
    //GPIO configuration
    ESP_LOGI(TAG, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

	gpio_pad_select_gpio(EXAMPLE_PIN_NUM_BK_LIGHT);
	gpio_pad_select_gpio(EXAMPLE_PIN_RD);
	gpio_pad_select_gpio(EXAMPLE_PIN_PWR);

	gpio_set_direction(EXAMPLE_PIN_RD, EXAMPLE_PIN_NUM_BK_LIGHT);
	gpio_set_direction(EXAMPLE_PIN_RD, GPIO_MODE_OUTPUT);
	gpio_set_direction(EXAMPLE_PIN_PWR, GPIO_MODE_OUTPUT);

    gpio_set_level(EXAMPLE_PIN_RD, true);
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL);

    ESP_LOGI(TAG, "Initialize Intel 8080 bus");
    esp_lcd_i80_bus_handle_t i80_bus = NULL;
    esp_lcd_i80_bus_config_t bus_config = {
        .dc_gpio_num = EXAMPLE_PIN_NUM_DC,
        .wr_gpio_num = EXAMPLE_PIN_NUM_PCLK,
		.clk_src	= LCD_CLK_SRC_PLL160M,
        .data_gpio_nums =
        {
            EXAMPLE_PIN_NUM_DATA0,
            EXAMPLE_PIN_NUM_DATA1,
            EXAMPLE_PIN_NUM_DATA2,
            EXAMPLE_PIN_NUM_DATA3,
            EXAMPLE_PIN_NUM_DATA4,
            EXAMPLE_PIN_NUM_DATA5,
            EXAMPLE_PIN_NUM_DATA6,
            EXAMPLE_PIN_NUM_DATA7,
        },
        .bus_width = 8,
        .max_transfer_bytes = LVGL_LCD_BUF_SIZE * sizeof(uint16_t)
    };
    ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus));
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = EXAMPLE_PIN_NUM_CS,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
        .trans_queue_depth = 20,
        .dc_levels = {
            .dc_idle_level = 0,
            .dc_cmd_level = 0,
            .dc_dummy_level = 0,
            .dc_data_level = 1,
        },
        .on_color_trans_done = example_notify_lvgl_flush_ready,
        .user_ctx = &disp_drv,
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install LCD driver of st7789");
    esp_lcd_panel_handle_t panel_handle = NULL;

    esp_lcd_panel_dev_config_t panel_config =
    {
        .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
        .color_space = ESP_LCD_COLOR_SPACE_RGB,
        .bits_per_pixel = 16,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_invert_color(panel_handle, true);

    esp_lcd_panel_swap_xy(panel_handle, true);
    esp_lcd_panel_mirror(panel_handle, false, true);
    // the gap is LCD panel specific, even panels with the same driver IC, can have different gap value
    esp_lcd_panel_set_gap(panel_handle, 0, 35);



    ESP_LOGI(TAG, "Turn on LCD backlight");
    gpio_set_level(EXAMPLE_PIN_PWR, true);
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);



    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();
    // alloc draw buffers used by LVGL
    // it's recommended to choose the size of the draw buffer(s) to be at least 1/10 screen sized
    lv_color_t *buf1 = heap_caps_malloc(LVGL_LCD_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA );
    assert(buf1);
//    lv_color_t *buf2 = heap_caps_malloc(LVGL_LCD_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA );
//    assert(buf2);
    // initialize LVGL draw buffers
    lv_disp_draw_buf_init(&disp_buf, buf1, NULL, LVGL_LCD_BUF_SIZE);

    ESP_LOGI(TAG, "Register display driver to LVGL");
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = EXAMPLE_LCD_H_RES;
    disp_drv.ver_res = EXAMPLE_LCD_V_RES;
    disp_drv.flush_cb = example_lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    //Configuration is completed.


    ESP_LOGI(TAG, "Install LVGL tick timer");
    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

    ESP_LOGI(TAG, "Display LVGL animation");
    lv_obj_t *scr = lv_disp_get_scr_act(disp);

    lvgl_demo_ui(scr);

    main_creatSysteTasks();



}
/**************************  Useful Electronics  ****************END OF FILE***/
