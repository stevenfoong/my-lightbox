#include "main.h"

#include "esp_err.h"
#include "esp_log.h"

#include "nvs_flash.h"



static const char *TAG = "NVS_Storage";

esp_err_t save_nvs_key_value(char * key, char * value)
{
	nvs_handle_t my_handle;
	esp_err_t err;

	// Open
	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;

	// Write
    ESP_LOGI(TAG, "Write key %s with value %s", key, value);
	err = nvs_set_str(my_handle, key, value);
	if (err != ESP_OK) return err;


	// Commit written value.
	// After setting any values, nvs_commit() must be called to ensure changes are written
	// to flash storage. Implementations may write to storage at other times,
	// but this is not guaranteed.
	err = nvs_commit(my_handle);
	if (err != ESP_OK) return err;

	// Close
	nvs_close(my_handle);
	return ESP_OK;
}

esp_err_t load_nvs_key_value(char * key, char * value, size_t size)
{
	nvs_handle_t my_handle;
	esp_err_t err;

	// Open
	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;

	// Read
	size_t _size = size;
	err = nvs_get_str(my_handle, key, value, &_size);
	ESP_LOGI(TAG, "nvs_get_str err=%d", err);
	//if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;
    if (err != ESP_OK) return err;
	ESP_LOGI(TAG, "err=%d key=[%s] value=[%s] _size=%d", err, key, value, _size);

	// Close
	nvs_close(my_handle);
	//return ESP_OK;
	return err;
}


esp_err_t save_wifi_key(char * key, char * value, size_t size)
{
	nvs_handle_t my_handle;
	esp_err_t err;

	// Open
	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;

	// Write
    
    ESP_LOGI(TAG, "Write key %s with value %s", key, value);
	err = nvs_set_blob(my_handle, key, value, size);
    //ESP_LOGI(TAG, " %d ", err);
	if (err != ESP_OK) return err;


	// Commit written value.
	// After setting any values, nvs_commit() must be called to ensure changes are written
	// to flash storage. Implementations may write to storage at other times,
	// but this is not guaranteed.
	err = nvs_commit(my_handle);
	if (err != ESP_OK) return err;

	// Close
	nvs_close(my_handle);
	return ESP_OK;
}