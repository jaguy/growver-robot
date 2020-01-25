//*****************************************************************************
//
// growver_main.c - Growver ESP32 Firmware
// License: GPL-3.0-or-later
// Copyright 2017 Revely Microsystems LLC.
//
//*****************************************************************************

//************************************************************************************************
// Notes:
// ESP32 partition table should be configured for 2 OTA + factory partition.
// Uses HTTPS
//
//************************************************************************************************

#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <mdns.h>
#include "freertos/event_groups.h"
#include "commandline.h"
#include "esp_spiffs.h"
#include "../components/motor/motor_dc.h"
#include "../components/motor/servo.h"
#include "../components/ws2812/ws2812.h"
#include "../components/other/peripheral.h"
#include "growver_mdns.h"
#include "esp_vfs.h"
#include <esp_http_server.h>
#include "ota-http.h"
#include "WebFiles/webpageassets.h"
#include "../components/prov/app_prov.h"
#include "file_server.h"
#include "growver_rest.h"


#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD
#define EXAMPLE_AP_RECONN_ATTEMPTS  CONFIG_EXAMPLE_AP_RECONN_ATTEMPTS

#define CONFIG_EXAMPLE_POP "abcd1234"

// FreeRTOS event group to signal when we are connected & ready to make a request.
static EventGroupHandle_t wifi_event_group;

// The event group allows multiple bits for each event, but we only care about one
// event - are we connected to the AP with an IP?
const int IP4_CONNECTED_BIT = BIT0;
const int IP6_CONNECTED_BIT = BIT1;

// Debug console tag for this code module
static const char *TAG="APP";

// GPIO Pin assignments
#define WS2812_PIN	25

// Delay macro
#define delay_ms(ms) vTaskDelay((ms) / portTICK_RATE_MS)

// Metadata on when this code module was built
const char *main_time = __TIME__;
const char *main_date = __DATE__;

//************************************************************************************************
// Serve root web page handler
//
//************************************************************************************************
esp_err_t root_get_handler(httpd_req_t *req)
{
    //ESP_LOGI(TAG, "Root URI");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
    return ESP_OK;
}

//************************************************************************************************
// Serve ota page
//
//************************************************************************************************
esp_err_t OTA_index_html_handler(httpd_req_t *req)
{
	// Clear this every time page is requested
	flash_status = 0;

    // Response with ota page
	httpd_resp_set_type(req, "text/html");
	httpd_resp_send(req, (const char *)ota_page_html_start, ota_page_html_end - ota_page_html_start);
	return ESP_OK;
}

//************************************************************************************************
// Serve Favicon
//
//************************************************************************************************
esp_err_t OTA_favicon_ico_handler(httpd_req_t *req)
{
	ESP_LOGI("OTA", "favicon_ico Requested");
	httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_end - favicon_ico_start);
	return ESP_OK;
}

//************************************************************************************************
// Start Web server
//
//************************************************************************************************
static httpd_handle_t start_webserver(const char *base_path)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Increase URI handlers from default
    config.max_uri_handlers = 10;

    static struct file_server_data *server_data = NULL;

    // Task for rebooting after firmware update
	xTaskCreate(&systemRebootTask, "rebootTask", 2048, NULL, 5, NULL);

    // Allocate memory for server data
    server_data = calloc(1, sizeof(struct file_server_data));
    if (!server_data)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for server data");
        return NULL;
    }
    strlcpy(server_data->base_path, base_path,
            sizeof(server_data->base_path));

    // Allocate memory for REST context
    rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));

    static const httpd_uri_t root =
    {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get_handler,
        .user_ctx = NULL
    };

    // URI handler for REST GET (fetching info)
    httpd_uri_t rest_get_uri =
    {
        .uri = "/api/v1/*",
        .method = HTTP_GET,
        .handler = rest_get_handler,
        .user_ctx = rest_context
    };

    // URI handler for REST POST (control)
    httpd_uri_t rest_post_uri =
    {
        .uri = "/api/v1/*",
        .method = HTTP_POST,
        .handler = rest_post_handler,
        .user_ctx = rest_context
    };

    static const httpd_uri_t OTA_favicon_ico =
    {
        .uri = "/favicon.ico",
        .method = HTTP_GET,
        .handler = OTA_favicon_ico_handler,
        .user_ctx = NULL
    };

    static const httpd_uri_t OTA_index =
    {
        .uri = "/ota-page*",
        .method = HTTP_GET,
        .handler = OTA_index_html_handler,
        .user_ctx = NULL
    };

    static const httpd_uri_t OTA_update =
    {
        .uri = "/update",
        .method = HTTP_POST,
        .handler = OTA_update_post_handler,
        .user_ctx = NULL
    };

    static const httpd_uri_t OTA_status =
    {
        .uri = "/status",
        .method = HTTP_POST,
        .handler = OTA_update_status_handler,
        .user_ctx = NULL
    };

    // URI handler for getting uploaded files
    // Match all URIs of type /path/to/file (was "/*")
    httpd_uri_t file_download =
    {
        .uri       = "/fs/*",
        .method    = HTTP_GET,
        .handler   = download_get_handler,
        .user_ctx  = server_data
    };

    // URI handler for uploading files to server
    // Match all URIs of type /upload/path/to/file
    httpd_uri_t file_upload =
    {
        .uri       = "/upload/*",
        .method    = HTTP_POST,
        .handler   = upload_post_handler,
        .user_ctx  = server_data
    };

    // URI handler for deleting files from server
    // Match all URIs of type /delete/path/to/file
    httpd_uri_t file_delete =
    {
        .uri       = "/delete/*",
        .method    = HTTP_POST,
        .handler   = delete_post_handler,
        .user_ctx  = server_data
    };

    // Enable wildcard URIs
    config.uri_match_fn = httpd_uri_match_wildcard;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &rest_get_uri);
        httpd_register_uri_handler(server, &rest_post_uri);
        httpd_register_uri_handler(server, &OTA_index);
        httpd_register_uri_handler(server, &OTA_update);
		httpd_register_uri_handler(server, &OTA_status);
        httpd_register_uri_handler(server, &OTA_favicon_ico);
        httpd_register_uri_handler(server, &file_download);
        httpd_register_uri_handler(server, &file_upload);
        httpd_register_uri_handler(server, &file_delete);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

//************************************************************************************************
// StartBLEProvisioning
//
//************************************************************************************************
static void StartBLEProvisioning()
{
    /* Security version */
    int security = 0;
    /* Proof of possession */
    const protocomm_security_pop_t *pop = NULL;

#ifdef CONFIG_EXAMPLE_USE_SEC_1
    security = 1;
#endif

    /* Having proof of possession is optional */
#ifdef CONFIG_EXAMPLE_USE_POP
    const static protocomm_security_pop_t app_pop = {
        .data = (uint8_t *) CONFIG_EXAMPLE_POP,
        .len = (sizeof(CONFIG_EXAMPLE_POP)-1)
    };
    pop = &app_pop;
#endif

    ESP_ERROR_CHECK(app_prov_start_ble_provisioning(security, pop));
}

//************************************************************************************************
// Stop Webserver
//
//************************************************************************************************
void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}

//************************************************************************************************
//
// Event handler for Wifi, Ethernet or IP events
//
//************************************************************************************************
static void event_handler(void* arg, esp_event_base_t event_base,
                          int event_id, void* event_data)
{
    httpd_handle_t server = NULL;
    static int s_retry_num_ap_not_found = 0;
    static int s_retry_num_ap_auth_fail = 0;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_event_sta_disconnected_t* disconnected = (wifi_event_sta_disconnected_t*) event_data;
        switch (disconnected->reason) {
        case WIFI_REASON_AUTH_EXPIRE:
        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
        case WIFI_REASON_BEACON_TIMEOUT:
        case WIFI_REASON_AUTH_FAIL:
        case WIFI_REASON_ASSOC_FAIL:
        case WIFI_REASON_HANDSHAKE_TIMEOUT:
            ESP_LOGW(TAG, "connect to the AP fail : auth Error");
            if (s_retry_num_ap_auth_fail < EXAMPLE_AP_RECONN_ATTEMPTS)
            {
                s_retry_num_ap_auth_fail++;
                esp_wifi_connect();
                ESP_LOGI(TAG, "retry connecting to the AP...");
            } else
            {
                // Restart provisioning if authentication fails
                StartBLEProvisioning();
            }
            break;
        case WIFI_REASON_NO_AP_FOUND:
            ESP_LOGW(TAG, "connect to the AP fail : not found");
            if (s_retry_num_ap_not_found < EXAMPLE_AP_RECONN_ATTEMPTS)
            {
                s_retry_num_ap_not_found++;
                esp_wifi_connect();
                ESP_LOGI(TAG, "retry to connecting to the AP...");
            }
            break;
        default:
            // None of the expected reasons
            esp_wifi_connect();
            break;
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->ip_info.ip));
        s_retry_num_ap_not_found = 0;
        s_retry_num_ap_auth_fail = 0;
        // Start Webserver
        if (server == NULL)
        {
            // TODO: Is this base path correct?
            server = start_webserver("/spiffs");
        }
    }
}

//************************************************************************************************
// Initialise SPIFFS
//
//************************************************************************************************
static esp_err_t init_spiffs(void)
{
    // .max_files sets the maximum number of files that can be created on the storage
    esp_vfs_spiffs_conf_t conf =
    {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    return ESP_OK;
}

//************************************************************************************************
// Initialise Wifi
//
//************************************************************************************************
static void wifi_init_sta()
{
    // Set network event handling
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler, NULL));

    // Start Wi-Fi in station mode with credentials set during provisioning
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

//************************************************************************************************
// Main application
//
//************************************************************************************************
void app_main()
{
    bool provisioned, led_on = 0;

    // Uart init for command line
    uart_init();

    // Motors ready and stopped
    MotorDCInit();

    // Initialise NVS flash storage for Wifi credentials etc.
    ESP_ERROR_CHECK(nvs_flash_init());

    // Initialize network stack
    wifi_event_group = xEventGroupCreate();
    tcpip_adapter_init();

    // Create default event loop needed by the app and the provisioning service
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    initialise_mdns();

    // Initialize Wi-Fi with default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Check if device is provisioned
    if (app_prov_is_provisioned(&provisioned) != ESP_OK)
    {
        ESP_LOGE(TAG, "Error getting device provisioning state");
        return;
    }
    if (provisioned == false)
    {
        // If not provisioned, start provisioning via BLE
        ESP_LOGI(TAG, "Starting BLE provisioning");
        StartBLEProvisioning();
    }
    else
    {
        // Else start as station with credentials set during provisioning
        ESP_LOGI(TAG, "Starting WiFi station");
        wifi_init_sta(NULL);
    }

    // Initialize file storage
    ESP_ERROR_CHECK(init_spiffs());

    // Initialize other controller functions
    ServoInit();
    AnalogMeasInit();
    PumpInit();
    ws2812_init(WS2812_PIN);

    // Main periodic loop
    // TODO: Change this to a dedicated periodic task?
    int blink_timer = 0;
    while(1)
    {
        // Change LED color to BLUE if either motor is running
        if (MotorDCGetSpeed(0) || MotorDCGetSpeed(1))
        {
            if (!led_on)
                ws2812_setColors(1, (rgbVal*)&ws2812_GRN);
        }
        else
        {
            if (blink_timer == 0)
                ws2812_setColors(1, (rgbVal*)&ws2812_OFF);
            // Blink?
            if (++blink_timer == 4)
            {
                ws2812_setColors(1, (rgbVal*)&ws2812_GRN);
                blink_timer = 0;
            }
        }

        // Run loop 4x per second
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}
