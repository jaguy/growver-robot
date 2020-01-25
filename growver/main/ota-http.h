//*****************************************************************************
// Adapter from ESP32-OTA-Webserver
// https://github.com/versamodule/ESP32-OTA-Webserver
//
//*****************************************************************************

void systemRebootTask(void * parameter);
esp_err_t OTA_update_status_handler(httpd_req_t *req);
esp_err_t OTA_update_post_handler(httpd_req_t *req);
void systemRebootTask(void * parameter);

extern int8_t flash_status;