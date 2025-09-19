#include <stdio.h>
#include <string.h>
#include <os/os.h>
#include "lingxin_http.h"
#include "components/log.h"
#include "components/webclient.h"
#include "components/system.h"


#define TAG "http"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

int http_post(HttpConfig *config, RequestCallback userCallback, void *userData)
{
    int ret = 0;
    int rx_buffer_size = 2*1024;
    int header_size = 500;
    char *uri = NULL;
    struct webclient_session* session = NULL;
    unsigned char *buffer = NULL;
    int bytes_read, resp_status;
    size_t data_len = 0;
    char *post_data = NULL;
    if (config->post_data) {
        data_len = strlen(config->post_data);
        post_data = web_malloc(data_len);
        memcpy(post_data, config->post_data, data_len); 
    }

    int url_len = strlen(config->protocol) + 3 + strlen(config->host) + 6 + strlen(config->path) + 1; // protocol://host:port/path
    uri = (char *)web_malloc(url_len);
    if (uri == NULL) {
        BK_LOGE(TAG, "Failed to allocate memory for URI\n");
        ret = -5;
        goto __exit;
    }
    snprintf(uri, url_len, "%s://%s:%d%s", config->protocol, config->host, config->port, config->path);

    buffer = (unsigned char *) web_malloc(rx_buffer_size);
    if (buffer == NULL)
    {
        BK_LOGE(TAG,"no memory for receive response buffer.\n");
        ret = -5;
        goto __exit;
    }

    /* create webclient session and set header response size */
    session = webclient_session_create(header_size);
    if (session == NULL)
    {
        ret = -5;
        goto __exit;
    }

    /* build header for upload */
    webclient_header_fields_add(session, "Content-Length: %d\r\n", strlen(post_data));
    webclient_header_fields_add(session, "Content-Type: application/json\r\n");
    
    if (config->headers) {
        webclient_header_fields_add(session, "signature: %s\r\n", config->headers->signature);
        webclient_header_fields_add(session, "sn: %s\r\n", config->headers->sn);
        webclient_header_fields_add(session, "app_id: %s\r\n", config->headers->app_id);
        webclient_header_fields_add(session, "timestamp: %s\r\n", config->headers->timestamp);
    }
    
    /* send POST request by default header */
    if (post_data && ((resp_status = webclient_post(session, uri, post_data, data_len)) != 200))
    {
        BK_LOGE(TAG,"webclient POST request failed, response(%d) error.\n", resp_status);
        ret = -1;
        goto __exit;
    }

    BK_LOGI(TAG,"webclient post response data: \n");
    do
    {
        bytes_read = webclient_read(session, buffer, rx_buffer_size);
        if (bytes_read > 0)
        {
            break;
        }
    }
    while (1);
    BK_LOGI(TAG, "bytes_read: %d\n", bytes_read);
    BK_LOGI(TAG, "buffer %s.\n", buffer);
    if (bytes_read > 0 && userData) {
        memcpy(userData, buffer, bytes_read);    //what is userData?
        ((char*)userData)[bytes_read] = '\0';
    }

    if (userCallback) {
        userCallback(buffer, bytes_read, userData);
    }

__exit:
    if (session)
    {
        webclient_close(session);
    }

    if (buffer)
    {
        web_free(buffer);
    }

    if (uri)
    {
        web_free(uri);
    }

    BK_LOGI(TAG,"http_post result(%d).\n", ret);
    return ret;
}


