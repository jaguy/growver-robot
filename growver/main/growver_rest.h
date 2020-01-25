// Growver REST Module header file

#define REST_SCRATCH_BUFSIZE (10240)

extern const char *URI_REST_API;

typedef struct rest_server_context
{
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[REST_SCRATCH_BUFSIZE];
} rest_server_context_t;

esp_err_t rest_post_handler(httpd_req_t *req);
esp_err_t rest_get_handler(httpd_req_t *req);
