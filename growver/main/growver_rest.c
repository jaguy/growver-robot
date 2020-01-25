//*****************************************************************************
//
// growver_rest.c - REST API for Growver Robot
//
// License: GPL-3.0-or-later
// Copyright 2017 Revely Microsystems LLC.
//
//
//*****************************************************************************
#include <string.h>
#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"
#include "growver_rest.h"
#include "../components/motor/motor_dc.h"
#include "../components/motor/servo.h"
#include "../components/other/peripheral.h"

static const char *REST_TAG = "rest";

const char *URI_REST_API = "/api/v1/*";

// Command prototypes
int ActionMotorSpeedPost(char *buf);
int ActionPumpControlPost(char *buf);
int ActionServoControlPost(char *buf);
int ActionStatusGet(cJSON *json_response);

// Typedef for Post and Get functions
typedef int (*pPostCmd)(char *buf);
typedef int (*pGetCmd)(cJSON *json_response);

// Define a structure for the POST command table
typedef struct
{
    // Name
    const char *pName;
    // Function to call.
    pPostCmd pCmd;
}
tPostCmdEntry;

// Define a structure for the GET command table
typedef struct
{
    // Name
    const char *pName;
    // Function to call.
    pGetCmd pCmd;
}
tGetCmdEntry;

// Table of Post API commands
tPostCmdEntry PostCmdTable[] =
{
	{ "motor", ActionMotorSpeedPost},
    { "pump", ActionPumpControlPost},
    { "servo", ActionServoControlPost},
    { 0, 0}
};

// Table of Get API commands
tGetCmdEntry GetCmdTable[] =
{
	{ "status", ActionStatusGet},
    { 0, 0}
};

//*****************************************************************************
// JsonGetIntItem
//
// Accepts pointer to CJSON object. Looks for Item with matching name and
// updates variable only if it exists.
//
//*****************************************************************************
bool JsonGetIntItem(cJSON *object, const char *item, int32_t *var)
{
    if (cJSON_GetObjectItem(object, item))
    {
        *var = cJSON_GetObjectItem(object, item)->valueint;
        return true;
    }
    return false;
}

//*****************************************************************************
// ActionMotorSpeedPost
//
//*****************************************************************************
int ActionMotorSpeedPost(char *buf)
{
    // Get current speed and direction
    int32_t ls = MotorDCGetSpeed(MOTOR_L);
    int32_t rs = MotorDCGetSpeed(MOTOR_R);
    int32_t ld = MotorDCGetDirection(MOTOR_L);
    int32_t rd = MotorDCGetDirection(MOTOR_R);

    // Parse JSON to update motor settings
    cJSON *root = cJSON_Parse(buf);
    if (root == NULL)
    {
        return ESP_FAIL;
    }
    JsonGetIntItem(root, "left_speed", &ls);
    JsonGetIntItem(root, "right_speed", &rs);
    JsonGetIntItem(root, "left_dir", &ld);
    JsonGetIntItem(root, "right_dir", &rd);
    cJSON_Delete(root);
	ESP_LOGI(REST_TAG, "Left %u %u Right %u %u\n", ls, ld, rs, rd);

	// Set the speed for both motors
	MotorDCSetSpeed(MOTOR_R, rs, rd);
	MotorDCSetSpeed(MOTOR_L, ls, ld);

	return (ESP_OK);
}

//*****************************************************************************
// ActionPumpControlPost
//
//*****************************************************************************
int ActionPumpControlPost(char *buf)
{
    cJSON *root = cJSON_Parse(buf);
    if (root == NULL)
    {
        return ESP_FAIL;
    }
    uint8_t speed = cJSON_GetObjectItem(root, "speed")->valueint;
    cJSON_Delete(root);

	ESP_LOGI(REST_TAG, "Pump %u\n", speed);

	(speed) ? (PumpControlSet(1)) : (PumpControlSet(0));

	return 0;
}

//*****************************************************************************
// ActionServoControlPost
//
//*****************************************************************************
int ActionServoControlPost(char *buf)
{
    cJSON *root = cJSON_Parse(buf);
    if (root == NULL)
    {
        return ESP_FAIL;
    }
    uint32_t angle = cJSON_GetObjectItem(root, "angle")->valueint;
    cJSON_Delete(root);

	ESP_LOGI(REST_TAG, "Servo %u\n", angle);
    ServoSetAngle(angle);

	return 0;
}

//*****************************************************************************
// GetStatus
//
//*****************************************************************************
int ActionStatusGet(cJSON *json_response)
{
    cJSON_AddNumberToObject(json_response, "battery_v", (double)AnalogVoltageRead() / 1000);
    return 0;
}

//*****************************************************************************
// ProcessPost
// Accepts a pointer to an API. Extracts the API (everything before first slash).
//
//*****************************************************************************
int ProcessPost(char *uri, char *content)
{
    tPostCmdEntry *psCmdEntry;

    //
    // Start at the beginning of the command table, to look for a matching
    // command.
    //
    psCmdEntry = &PostCmdTable[0];

    //
    // Search through the command table until a null command string is
    // found, which marks the end of the table.
    //
    while(psCmdEntry->pCmd)
    {
        // Is there a match? If so then call the corresponding function.
        if(!strcmp(uri, psCmdEntry->pName))
        {
	    	//ESP_LOGI(REST_TAG, "Cmd:%s\n", uri);
            return(psCmdEntry->pCmd(content));
        }

        psCmdEntry++;
    }

    // No matches
    return 0;
}

//*****************************************************************************
// ProcessGet
// Accepts a pointer to an API and a pointer to a JSON object. Inserts
// the matching GET response into the JSON object.
//
//*****************************************************************************
int ProcessGet(char *uri, cJSON *json_response)
{
    tGetCmdEntry *psCmdEntry;

    //
    // Start at the beginning of the command table, to look for a matching
    // command.
    //
    psCmdEntry = &GetCmdTable[0];

    //
    // Search through the command table until a null command string is
    // found, which marks the end of the table.
    //
    while(psCmdEntry->pCmd)
    {
        // Is there a match? If so then call the corresponding function.
        if(!strcmp(uri, psCmdEntry->pName))
        {
	    	//ESP_LOGI(REST_TAG, "Cmd:%s\n", uri);
            return(psCmdEntry->pCmd(json_response));
        }

        psCmdEntry++;
    }

    // No matches
    return 0;
}

//*****************************************************************************
// Handler for REST Post
//
//*****************************************************************************
esp_err_t rest_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    char *api;

    if (total_len >= REST_SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    // Skip past REST root URI (less 1 for asterisk)
    api = (char*)req->uri + strlen(URI_REST_API) - 1;
    //ESP_LOGI(REST_TAG, "URI [%s]Post to [%s] with [%s]\n", req->uri, api , buf);

    // Process the Post
    ProcessPost(api, buf);

    httpd_resp_sendstr(req, "Post control value successfully");
    return ESP_OK;
}

#if 0
/* REST Get handler */
static esp_err_t rest_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    cJSON_AddStringToObject(root, "version", IDF_VER);
    cJSON_AddNumberToObject(root, "cores", chip_info.cores);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}
#endif

//*****************************************************************************
// Handler for REST Get
//
//*****************************************************************************
esp_err_t rest_get_handler(httpd_req_t *req)
{
    char *api;

    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();

    // Skip past REST root URI (less 1 for asterisk)
    api = (char*) req->uri + strlen(URI_REST_API) - 1;

    // Process the Get
    ProcessGet(api, root);

    //cJSON_AddNumberToObject(root, "raw", esp_random() % 20);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}
