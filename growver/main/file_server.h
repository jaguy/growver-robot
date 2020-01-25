// File server header file
#define SCRATCH_BUFSIZE  8192

struct file_server_data {
    /* Base path of file storage */
    char base_path[ESP_VFS_PATH_MAX + 1];

    /* Scratch buffer for temporary storage during file transfer */
    char scratch[SCRATCH_BUFSIZE];
};

/* Declare the function which starts the file server.
 * Implementation of this function is to be found in
 * file_server.c */
esp_err_t start_file_server(const char *base_path);

esp_err_t index_html_get_handler(httpd_req_t *req);
//static esp_err_t http_resp_dir_html(httpd_req_t *req, const char *dirpath);
//static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename);
//static const char* get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize);
esp_err_t download_get_handler(httpd_req_t *req);
esp_err_t upload_post_handler(httpd_req_t *req);
esp_err_t delete_post_handler(httpd_req_t *req);