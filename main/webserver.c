//#include "webserver.h"
#include "main.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_event.h"

#include "esp_wifi.h"
#include <esp_http_server.h>

static const char *TAG = "WebServer";

/* function pointers to URI handlers that can be user made */
esp_err_t (*custom_get_httpd_uri_handler)(httpd_req_t *r) = NULL;
esp_err_t (*custom_post_httpd_uri_handler)(httpd_req_t *r) = NULL;

/* strings holding the URLs of the wifi manager */
static char* http_root_url = "/";
static char* http_redirect_url = NULL;
static char* http_js_url = NULL;
static char* http_css_url = NULL;
static char* http_connect_url = NULL;
static char* http_ap_url = NULL;
static char* http_status_url = NULL;

/**
 * @brief embedded binary data.
 * @see file "component.mk"
 * @see https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html#embedding-binary-data
 */
extern const uint8_t style_css_start[] asm("_binary_style_css_start");
extern const uint8_t style_css_end[]   asm("_binary_style_css_end");
extern const uint8_t code_js_start[] asm("_binary_code_js_start");
extern const uint8_t code_js_end[] asm("_binary_code_js_end");
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");

/* const httpd related values stored in ROM */
const static char http_200_hdr[] = "200 OK";
//const static char http_302_hdr[] = "302 Found";
const static char http_400_hdr[] = "400 Bad Request";
const static char http_404_hdr[] = "404 Not Found";
//const static char http_503_hdr[] = "503 Service Unavailable";
//const static char http_location_hdr[] = "Location";
const static char http_content_type_html[] = "text/html";
const static char http_content_type_js[] = "text/javascript";
const static char http_content_type_css[] = "text/css";
const static char http_content_type_json[] = "application/json";
const static char http_cache_control_hdr[] = "Cache-Control";
const static char http_cache_control_no_cache[] = "no-store, no-cache, must-revalidate, max-age=0";
const static char http_cache_control_cache[] = "public, max-age=31536000";
const static char http_pragma_hdr[] = "Pragma";
const static char http_pragma_no_cache[] = "no-cache";

char on_resp[] = "<!DOCTYPE html><html><head>"
                 "<style type=\"text/css\">"
                 "html {  font-family: Arial;  display: inline-block;  margin: 0px auto;  text-align: center;}h1{  color: #070812;  padding: 2vh;}."
                 "button {  display: inline-block;  background-color: #b30000; //red color  border: none;  border-radius: 4px;  color: white;  "
                 "padding: 16px 40px;  text-decoration: none;  font-size: 30px;  margin: 2px;  cursor: pointer;}.button2 {  background-color: #364cf4; "
                 "//blue color}.content {   padding: 50px;}.card-grid {  max-width: 800px;  margin: 0 auto;  display: grid;  grid-gap: 2rem;  "
                 "grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));}.card {  background-color: white;  box-shadow: 2px 2px 12px 1px "
                 "rgba(140,140,140,.5);}.card-title {  font-size: 1.2rem;  font-weight: bold;  color: #034078}</style>  "
                 "<title>ESP32 WEB SERVER</title>  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">  "
                 "<link rel=\"icon\" href=\"data:,\">  <link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\"    "
                 "integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\">  "
                 "<link rel=\"stylesheet\" type=\"text/css\" ></head><body>  <h2>ESP32 WEB SERVER</h2>  <div class=\"content\">  "
                 "  <div class=\"card-grid\">      <div class=\"card\">        <p><i class=\"fas fa-lightbulb fa-2x\" style=\"color:#c81919;\"></i>   "
                 "  <strong>GPIO2</strong></p>        <p>GPIO state: <strong> ON</strong></p>        <p>       "
                 "   <a href=\"/led2on\"><button class=\"button\">ON</button></a>          <a href=\"/led2off\"><button class=\"button button2\">OFF</button></a>    "
                 "    </p>      </div>    </div>  </div></body></html>";

char off_resp[] = "<!DOCTYPE html><html><head><style type=\"text/css\">html {  font-family: Arial;  display: inline-block;  margin: 0px auto;  text-align: center;}h1{  color: #070812;  padding: 2vh;}.button {  display: inline-block;  background-color: #b30000; //red color  border: none;  border-radius: 4px;  color: white;  padding: 16px 40px;  text-decoration: none;  font-size: 30px;  margin: 2px;  cursor: pointer;}.button2 {  background-color: #364cf4; //blue color}.content {   padding: 50px;}.card-grid {  max-width: 800px;  margin: 0 auto;  display: grid;  grid-gap: 2rem;  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));}.card {  background-color: white;  box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);}.card-title {  font-size: 1.2rem;  font-weight: bold;  color: #034078}</style>  <title>ESP32 WEB SERVER</title>  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">  <link rel=\"icon\" href=\"data:,\">  <link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\"    integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\">  <link rel=\"stylesheet\" type=\"text/css\"></head><body>  <h2>ESP32 WEB SERVER</h2>  <div class=\"content\">    <div class=\"card-grid\">      <div class=\"card\">        <p><i class=\"fas fa-lightbulb fa-2x\" style=\"color:#c81919;\"></i>     <strong>GPIO2</strong></p>        <p>GPIO state: <strong> OFF</strong></p>        <p>          <a href=\"/led2on\"><button class=\"button\">ON</button></a>          <a href=\"/led2off\"><button class=\"button button2\">OFF</button></a>        </p>      </div>    </div>  </div></body></html>";

esp_err_t send_web_page(httpd_req_t *req)
{
    int response;
    /*
    if (led_state == 0)
        response = httpd_resp_send(req, off_resp, HTTPD_RESP_USE_STRLEN);
    else
        response = httpd_resp_send(req, on_resp, HTTPD_RESP_USE_STRLEN);
    */
    //response = httpd_resp_send(req, on_resp, HTTPD_RESP_USE_STRLEN);
    response = httpd_resp_send(req, (char*)index_html_start, index_html_end - index_html_start);
    return response;
}
esp_err_t get_root_handler(httpd_req_t *req)
{
    httpd_resp_set_status(req, http_200_hdr);
	httpd_resp_set_type(req, http_content_type_html);
	httpd_resp_send(req, (char*)index_html_start, index_html_end - index_html_start);
    return ESP_OK;
    //return send_web_page(req);
}

esp_err_t get_style_css_handler(httpd_req_t *req)
{
    //int response;
    //response = httpd_resp_send(req, (char*)style_css_start, style_css_end - style_css_start);
    httpd_resp_set_status(req, http_200_hdr);
	httpd_resp_set_type(req, http_content_type_js);
	httpd_resp_send(req, (char*)style_css_start, style_css_end - style_css_start);
    return ESP_OK;
}

esp_err_t get_code_js_handler(httpd_req_t *req)
{
    //int response;
    //response = httpd_resp_send(req, (char*)code_js_start, code_js_end - code_js_start);
    httpd_resp_set_status(req, http_200_hdr);
	httpd_resp_set_type(req, http_content_type_css);
	httpd_resp_set_hdr(req, http_cache_control_hdr, http_cache_control_cache);
    httpd_resp_send(req, (char*)code_js_start, code_js_end - code_js_start);
    return ESP_OK;
}

esp_err_t post_save_wifi_handler(httpd_req_t *req)
{
    size_t ssid_len = 0, password_len = 0;
	char *ssid = NULL, *password = NULL;

	/* len of values provided */
	ssid_len = httpd_req_get_hdr_value_len(req, "X-Custom-ssid");
	password_len = httpd_req_get_hdr_value_len(req, "X-Custom-pwd");

    ssid = malloc(sizeof(char) * (ssid_len + 1));
	password = malloc(sizeof(char) * (password_len + 1));
	httpd_req_get_hdr_value_str(req, "X-Custom-ssid", ssid, ssid_len+1);
	httpd_req_get_hdr_value_str(req, "X-Custom-pwd", password, password_len+1);

    ESP_LOGI(TAG, "Save Wifi Credential");
    
    esp_err_t err = save_wifi_key("wifi_ssid", ssid, 32);
    if (err != ESP_OK) return err;
    err = save_wifi_key("wifi_password", password, 64);
    if (err != ESP_OK) return err;
    
    //save_nvs_key_value("wifi_ssid", ssid);
    //save_nvs_key_value("wifi_password", password);


	/* free memory */
	free(ssid);
	free(password);

	httpd_resp_set_status(req, http_200_hdr);
	httpd_resp_set_type(req, http_content_type_json);
	httpd_resp_set_hdr(req, http_cache_control_hdr, http_cache_control_no_cache);
	httpd_resp_set_hdr(req, http_pragma_hdr, http_pragma_no_cache);
	httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

esp_err_t restart_unit_handler(httpd_req_t *req)
{
    //gpio_set_level(LED_PIN, 1);
    //led_state = 1;
    //return send_web_page(req);
    httpd_resp_set_status(req, http_200_hdr);
	httpd_resp_set_type(req, http_content_type_html);
	httpd_resp_send(req, NULL, 0);

    esp_restart();

    return ESP_OK;
}


esp_err_t led_on_handler(httpd_req_t *req)
{
    //gpio_set_level(LED_PIN, 1);
    //led_state = 1;
    return send_web_page(req);
}

esp_err_t led_off_handler(httpd_req_t *req)
{
    //gpio_set_level(LED_PIN, 0);
    //led_state = 0;
    return send_web_page(req);
}

httpd_uri_t root_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_root_handler,
    .user_ctx = NULL};

httpd_uri_t style_css_get = {
    .uri = "/style.css",
    .method = HTTP_GET,
    .handler = get_style_css_handler,
    .user_ctx = NULL};

httpd_uri_t code_js_get = {
    .uri = "/code.js",
    .method = HTTP_GET,
    .handler = get_code_js_handler,
    .user_ctx = NULL};

httpd_uri_t save_wifi_json_post = {
    .uri = "/save_wifi.json",
    .method = HTTP_POST,
    .handler = post_save_wifi_handler,
    .user_ctx = NULL};

httpd_uri_t reset_unit_get = {
    .uri = "/restart_unit",
    .method = HTTP_GET,
    .handler = restart_unit_handler,
    .user_ctx = NULL};

httpd_uri_t uri_on = {
    .uri = "/led2on",
    .method = HTTP_GET,
    .handler = led_on_handler,
    .user_ctx = NULL};

httpd_uri_t uri_off = {
    .uri = "/led2off",
    .method = HTTP_GET,
    .handler = led_off_handler,
    .user_ctx = NULL};

/**
 * @brief helper to generate URLs of the wifi manager
 */
static char* http_app_generate_url(const char* page){

	char* ret;

	int root_len = strlen("/");
	const size_t url_sz = sizeof(char) * ( (root_len+1) + ( strlen(page) + 1) );

	ret = malloc(url_sz);
	memset(ret, 0x00, url_sz);
	strcpy(ret, "/");
	ret = strcat(ret, page);

	return ret;
}

static esp_err_t http_server_get_handler(httpd_req_t *req){

    char* host = NULL;
    size_t buf_len;
    esp_err_t ret = ESP_OK;

    ESP_LOGI(TAG, "Get Handler");
    ESP_LOGD(TAG, "GET %s", req->uri);

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
    	host = malloc(buf_len);
    	if(httpd_req_get_hdr_value_str(req, "Host", host, buf_len) != ESP_OK){
    		/* if something is wrong we just 0 the whole memory */
    		memset(host, 0x00, buf_len);
    	}
    }

	/* GET /  */
	if(strcmp(req->uri, "/") == 0){
		httpd_resp_set_status(req, http_200_hdr);
		httpd_resp_set_type(req, http_content_type_html);
		httpd_resp_send(req, (char*)index_html_start, index_html_end - index_html_start);
	}
	/* GET /code.js */
	else if(strcmp(req->uri, http_js_url) == 0){
		httpd_resp_set_status(req, http_200_hdr);
		httpd_resp_set_type(req, http_content_type_js);
		httpd_resp_send(req, (char*)code_js_start, code_js_end - code_js_start);
	}
	/* GET /style.css */
	else if(strcmp(req->uri, http_css_url) == 0){
		httpd_resp_set_status(req, http_200_hdr);
		httpd_resp_set_type(req, http_content_type_css);
		httpd_resp_set_hdr(req, http_cache_control_hdr, http_cache_control_cache);
		httpd_resp_send(req, (char*)style_css_start, style_css_end - style_css_start);
	}






    /* memory clean up */
    if(host != NULL){
    	free(host);
    }

    return ret;

}            

static esp_err_t http_server_post_handler(httpd_req_t *req){


	esp_err_t ret = ESP_OK;

	ESP_LOGI(TAG, "POST %s", req->uri);

	/* POST /connect.json */
	if(strcmp(req->uri, http_connect_url) == 0){


		/* buffers for the headers */
		size_t ssid_len = 0, password_len = 0;
		char *ssid = NULL, *password = NULL;

		/* len of values provided */
		ssid_len = httpd_req_get_hdr_value_len(req, "X-Custom-ssid");
		password_len = httpd_req_get_hdr_value_len(req, "X-Custom-pwd");


		if(ssid_len && ssid_len <= 32 && password_len && password_len <= 64){

			/* get the actual value of the headers */
			ssid = malloc(sizeof(char) * (ssid_len + 1));
			password = malloc(sizeof(char) * (password_len + 1));
			httpd_req_get_hdr_value_str(req, "X-Custom-ssid", ssid, ssid_len+1);
			httpd_req_get_hdr_value_str(req, "X-Custom-pwd", password, password_len+1);

			//wifi_config_t* config = wifi_manager_get_wifi_sta_config();
			//memset(config, 0x00, sizeof(wifi_config_t));
			//memcpy(config->sta.ssid, ssid, ssid_len);
			//memcpy(config->sta.password, password, password_len);
			ESP_LOGI(TAG, "ssid: %s, password: %s", ssid, password);
			ESP_LOGD(TAG, "http_server_post_handler: wifi_manager_connect_async() call");
			//wifi_manager_connect_async();

			/* free memory */
			free(ssid);
			free(password);

			httpd_resp_set_status(req, http_200_hdr);
			httpd_resp_set_type(req, http_content_type_json);
			httpd_resp_set_hdr(req, http_cache_control_hdr, http_cache_control_no_cache);
			httpd_resp_set_hdr(req, http_pragma_hdr, http_pragma_no_cache);
			httpd_resp_send(req, NULL, 0);

		}
		else{
			/* bad request the authentification header is not complete/not the correct format */
			httpd_resp_set_status(req, http_400_hdr);
			httpd_resp_send(req, NULL, 0);
		}

	}
	else{

		if(custom_post_httpd_uri_handler == NULL){
			httpd_resp_set_status(req, http_404_hdr);
			httpd_resp_send(req, NULL, 0);
		}
		else{

			/* if there's a hook, run it */
			ret = (*custom_post_httpd_uri_handler)(req);
		}
	}

	return ret;
}


/* URI wild card for any GET request */
static const httpd_uri_t http_server_get_request = {
    .uri       = "*",
    .method    = HTTP_GET,
    .handler   = http_server_get_handler
};

static const httpd_uri_t http_server_post_request = {
	.uri	= "*",
	.method = HTTP_POST,
	.handler = http_server_post_handler
};


httpd_handle_t setup_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    ESP_LOGI(TAG, "Starting Web Server");

	if(http_root_url == NULL){
		int root_len = strlen("/");

		/* all the pages */
		const char page_js[] = "code.js";
		const char page_css[] = "style.css";
		const char page_connect[] = "connect.json";
		const char page_ap[] = "ap.json";
		const char page_status[] = "status.json";

		/* root url, eg "/"   */
		const size_t http_root_url_sz = sizeof(char) * (root_len+1);
		http_root_url = malloc(http_root_url_sz);
		memset(http_root_url, 0x00, http_root_url_sz);
		strcpy(http_root_url, "/");

		/* redirect url */
		size_t redirect_sz = 22 + root_len + 1; /* strlen(http://255.255.255.255) + strlen("/") + 1 for \0 */
		http_redirect_url = malloc(sizeof(char) * redirect_sz);
		*http_redirect_url = '\0';

		if(root_len == 1){
			snprintf(http_redirect_url, redirect_sz, "http://%s", "192.168.1.75");
		}
		else{
			snprintf(http_redirect_url, redirect_sz, "http://%s%s", "192.168.1.75", "/");
		}

		/* generate the other pages URLs*/
		http_js_url = http_app_generate_url(page_js);
		http_css_url = http_app_generate_url(page_css);
		http_connect_url = http_app_generate_url(page_connect);
		http_ap_url = http_app_generate_url(page_ap);
		http_status_url = http_app_generate_url(page_status);

	}    

    if (httpd_start(&server, &config) == ESP_OK)
    {
        ESP_LOGI(TAG, "Registering URI handlers");
	    httpd_register_uri_handler(server, &http_server_get_request);
	    httpd_register_uri_handler(server, &http_server_post_request);
	    //httpd_register_uri_handler(server, &http_server_delete_request);
        httpd_register_uri_handler(server, &root_get);
        httpd_register_uri_handler(server, &style_css_get);
        httpd_register_uri_handler(server, &code_js_get);
        httpd_register_uri_handler(server, &save_wifi_json_post);
        httpd_register_uri_handler(server, &reset_unit_get);
        //httpd_register_uri_handler(server, &save_wifi_post);
        //httpd_register_uri_handler(server, &uri_on);
        //httpd_register_uri_handler(server, &uri_off);
    }

    return server;
}

