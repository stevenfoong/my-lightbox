#include "main.h"

#include "esp_flash.h"
#include "esp_flash_spi_init.h"
#include "esp_partition.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"

#include "esp_err.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>

#include <sys/unistd.h>
#include <sys/stat.h>

static const char *TAG = "storage";

// Handle of the wear levelling library instance
//static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

// Mount path for the partition
//const char *base_path = "/data";

void mount_stock_storage(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/stock",
      .partition_label = "stock",
      .max_files = 5,
      .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

#ifdef CONFIG_EXAMPLE_SPIFFS_CHECK_ON_START
    ESP_LOGI(TAG, "Performing SPIFFS_check().");
    ret = esp_spiffs_check(conf.partition_label);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
        return;
    } else {
        ESP_LOGI(TAG, "SPIFFS_check() successful");
    }
#endif

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(conf.partition_label);
        return;
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

        if (used > total) {
        ESP_LOGW(TAG, "Number of used bytes cannot be larger than total. Performing SPIFFS_check().");
        ret = esp_spiffs_check(conf.partition_label);
        // Could be also used to mend broken files, to clean unreferenced pages, etc.
        // More info at https://github.com/pellepl/spiffs/wiki/FAQ#powerlosses-contd-when-should-i-run-spiffs_check
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
            return;
        } else {
            ESP_LOGI(TAG, "SPIFFS_check() successful");
        }
    }

    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(TAG, "Opening file");
    FILE* f = fopen("/stock/webserver/webserver.txt", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }
    char line[64];
    fgets(line, sizeof(line), f);
    fclose(f);
    // strip newline
    //char* pos = strchr(line, '\n');
    char* pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);


    //FILE* f = fopen("/stock/test.txt", "w");
    f = fopen("/stock/test.txt", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }

    fgets(line, sizeof(line), f);
    fclose(f);
    // strip newline
    //char* pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);

    //fprintf(f, "Hello World!\n");
    //fclose(f);
    //ESP_LOGI(TAG, "File written");

    // Check if destination file exists before renaming
    struct stat st;
    if (stat("/spiffs/foo.txt", &st) == 0) {
        // Delete it if it exists
        unlink("/spiffs/foo.txt");
    }


}

