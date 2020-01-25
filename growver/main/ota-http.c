//*****************************************************************************
// Adapter from ESP32-OTA-Webserver
// https://github.com/versamodule/ESP32-OTA-Webserver
//
//*****************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <esp_log.h>
#include <sys/param.h>
#include "esp_ota_ops.h"
#include "esp_http_server.h"
#include "freertos/event_groups.h"

int8_t flash_status = 0;

EventGroupHandle_t reboot_event_group;
const int REBOOT_BIT = BIT0;

extern const char *main_time, *main_date;


/*****************************************************

	systemRebootTask()

	NOTES: This had to be a task because the web page needed
			an ack back. So i could not call this in the handler

 *****************************************************/
void systemRebootTask(void * parameter)
{

	// Init the event group
	reboot_event_group = xEventGroupCreate();

	// Clear the bit
	xEventGroupClearBits(reboot_event_group, REBOOT_BIT);


	for (;;)
	{
		// Wait here until the bit gets set for reboot
		EventBits_t staBits = xEventGroupWaitBits(reboot_event_group, REBOOT_BIT, pdTRUE, pdFALSE, portMAX_DELAY);

		// Did portMAX_DELAY ever timeout, not sure so lets just check to be sure
		if ((staBits & REBOOT_BIT) != 0)
		{
			ESP_LOGI("OTA", "Reboot Command, Restarting");
			vTaskDelay(2000 / portTICK_PERIOD_MS);

			esp_restart();
		}
	}
}

/* Status */
esp_err_t OTA_update_status_handler(httpd_req_t *req)
{
	char ledJSON[100];

	ESP_LOGI("OTA", "Status Requested");

	sprintf(ledJSON, "{\"status\":%d,\"compile_time\":\"%s\",\"compile_date\":\"%s\"}", flash_status, main_time, main_date);

	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, ledJSON, strlen(ledJSON));

	if (flash_status == 1)
	{
		// We cannot directly call reboot here because we need the
		// browser to get the ack back.
		xEventGroupSetBits(reboot_event_group, REBOOT_BIT);
	}

	return ESP_OK;
}
/* Receive .Bin file */
esp_err_t OTA_update_post_handler(httpd_req_t *req)
{
	esp_ota_handle_t ota_handle;

	char ota_buff[1024];
	int content_length = req->content_len;
	int content_received = 0;
	int recv_len;
	bool is_req_body_started = false;
	const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

	// Unsucessful Flashing
	flash_status = -1;

	do
	{
		/* Read the data for the request */
		if ((recv_len = httpd_req_recv(req, ota_buff, MIN(content_length, sizeof(ota_buff)))) < 0)
		{
			if (recv_len == HTTPD_SOCK_ERR_TIMEOUT)
			{
				ESP_LOGI("OTA", "Socket Timeout");
				/* Retry receiving if timeout occurred */
				continue;
			}
			ESP_LOGI("OTA", "OTA Other Error %d", recv_len);
			return ESP_FAIL;
		}

		printf("OTA RX: %d of %d\r", content_received, content_length);

	    // Is this the first data we are receiving
		// If so, it will have the information in the header we need.
		if (!is_req_body_started)
		{
			is_req_body_started = true;

			// Lets find out where the actual data staers after the header info
			char *body_start_p = strstr(ota_buff, "\r\n\r\n") + 4;
			int body_part_len = recv_len - (body_start_p - ota_buff);

			//int body_part_sta = recv_len - body_part_len;
			//printf("OTA File Size: %d : Start Location:%d - End Location:%d\r\n", content_length, body_part_sta, body_part_len);
			printf("OTA File Size: %d\r\n", content_length);

			esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
			if (err != ESP_OK)
			{
				printf("Error With OTA Begin, Cancelling OTA\r\n");
				return ESP_FAIL;
			}
			else
			{
				printf("Writing to partition subtype %d at offset 0x%x\r\n", update_partition->subtype, update_partition->address);
			}

			// Lets write this first part of data out
			esp_ota_write(ota_handle, body_start_p, body_part_len);
		}
		else
		{
			// Write OTA data
			esp_ota_write(ota_handle, ota_buff, recv_len);

			content_received += recv_len;
		}

	} while (recv_len > 0 && content_received < content_length);

	if (esp_ota_end(ota_handle) == ESP_OK)
	{
		// Lets update the partition
		if(esp_ota_set_boot_partition(update_partition) == ESP_OK)
		{
			const esp_partition_t *boot_partition = esp_ota_get_boot_partition();

			// Webpage will request status when complete
			// This is to let it know it was successful
			flash_status = 1;

			ESP_LOGI("OTA", "Next boot partition subtype %d at offset 0x%x", boot_partition->subtype, boot_partition->address);
			ESP_LOGI("OTA", "Please Restart System...");
		}
		else
		{
			ESP_LOGI("OTA", "\r\n\r\n !!! Flashed Error !!!");
		}

	}
	else
	{
		ESP_LOGI("OTA", "\r\n\r\n !!! OTA End Error !!!");
	}

	return ESP_OK;

}
