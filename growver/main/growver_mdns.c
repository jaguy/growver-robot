//*****************************************************************************
//
// growver_mdns.c - Growver ESP32 Firmware
// License: GPL-3.0-or-later
// Copyright 2017 Revely Microsystems LLC.
//
//*****************************************************************************

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mdns.h"
#include "driver/gpio.h"
#include <sys/socket.h>
#include <netdb.h>

#define EXAMPLE_MDNS_INSTANCE CONFIG_MDNS_INSTANCE

static const char *TAG = "mdns-test";
static char* generate_hostname();

void initialise_mdns(void)
{
    char* hostname = generate_hostname();
    //initialize mDNS
    ESP_ERROR_CHECK( mdns_init() );
    //set mDNS hostname (required if you want to advertise services)
    ESP_ERROR_CHECK( mdns_hostname_set(hostname) );
    ESP_LOGI(TAG, "mdns hostname set to: [%s]", hostname);
    //set default mDNS instance name
    //ESP_ERROR_CHECK( mdns_instance_name_set(EXAMPLE_MDNS_INSTANCE) );
    ESP_ERROR_CHECK( mdns_instance_name_set("growver") );

    //structure with TXT records
    mdns_txt_item_t serviceTxtData[3] = {
        {"board","esp32"},
        {"u","user"},
        {"p","password"}
    };

    //initialize service
    //ESP_ERROR_CHECK( mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, serviceTxtData, 3) );
    ESP_ERROR_CHECK( mdns_service_add(NULL, "_http", "_tcp", 80, serviceTxtData, 3) );

    free(hostname);
}


static char* generate_hostname()
{
#ifndef CONFIG_MDNS_ADD_MAC_TO_HOSTNAME
    return strdup("growver");
    //return strdup(CONFIG_MDNS_HOSTNAME);
#else
    uint8_t mac[6];
    char   *hostname;
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (-1 == asprintf(&hostname, "%s-%02X%02X%02X", CONFIG_MDNS_HOSTNAME, mac[3], mac[4], mac[5])) {
        abort();
    }
    return hostname;
#endif
}
