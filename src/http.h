#ifndef OB_HTTP_H
#define OB_HTTP_H

#include <curl/curl.h>
#include <stdbool.h>
#include <time.h>
#include <stdint.h>

#include "error.h"
#include "buffer.h"
#include "body.h"
#include "header.h"
#include "queryparams.h"

struct OB_ProgressData;

typedef bool (*OB_ProgressCallback)(size_t accumulated, size_t total, struct OB_ProgressData*);

struct OB_ProgressData {
    OB_ProgressCallback callback;
    void               *user_data;
    size_t              accumulated;
    int64_t             start_time;
    int64_t             current_time;
};

struct OB_Progress {
    struct OB_ProgressData download;
    struct OB_ProgressData upload;
};

enum OB_Http_Method {
    OB_HTTP_METHOD_GET,
    OB_HTTP_METHOD_POST,
    OB_HTTP_METHOD_HEAD,
    OB_HTTP_METHOD_PUT,
    OB_HTTP_METHOD_PATCH,
    OB_HTTP_METHOD_DELETE
};

struct OB_Http_Client {
    CURL              *curl;
    unsigned           max_redirections;
    bool               get_headers;
    bool               get_body;
    struct OB_Progress progress;
    char               error[CURL_ERROR_SIZE];
};

struct OB_Url {
    const char                *value;
    struct OB_Http_QueryParams query_params;
};

struct OB_Http_Request {
    struct OB_Url           url;
    struct OB_Http_Body     body;
    enum OB_Http_Method     method;
    struct OB_Http_Headers  headers;
    bool                    follow_redirections;
    struct curl_slist      *curl_headers;
};

struct OB_Http_Response {
    struct OB_Http_Body    body;
    struct OB_Http_Headers headers;
    unsigned               status_code;
};

bool               OB_Http_init                         (void); 
void               OB_Http_free                         (void); 
bool               OB_Http_Client_init                  (struct OB_Http_Client*);
void               OB_Http_Client_free                  (struct OB_Http_Client*);
enum OB_Http_Error OB_Http_Client_fetch                 (struct OB_Http_Client*, struct OB_Http_Request*, struct OB_Http_Response*);
bool               OB_Http_Client_get_headers           (struct OB_Http_Client*, struct OB_Http_Response*);
void               OB_Http_Client_on_upload             (struct OB_Http_Client*, OB_ProgressCallback, void *user_data);
void               OB_Http_Client_on_download           (struct OB_Http_Client*, OB_ProgressCallback, void *user_data);
void               OB_Http_Client_show_upload_progress  (struct OB_Http_Client*);
void               OB_Http_Client_show_download_progress(struct OB_Http_Client*);
const char        *OB_Http_Client_get_error             (const struct OB_Http_Client*);

void OB_Http_Request_init                    (struct OB_Http_Request*);
void OB_Http_Request_free                    (struct OB_Http_Request*);
void OB_Http_Request_set_url                 (struct OB_Http_Request*, const char*);
bool OB_Http_Request_set_query_param         (struct OB_Http_Request*, const char *name, const char *value);
bool OB_Http_Request_basic_auth              (struct OB_Http_Request*, const char *username, const char *password);
void OB_Http_Request_set_file                (struct OB_Http_Request*, FILE*);
bool OB_Http_Request_set_file_path           (struct OB_Http_Request*, const char*);
struct OB_Http_Body *OB_Http_Request_get_body(struct OB_Http_Request*);

void                 OB_Http_Response_init           (struct OB_Http_Response*);
void                 OB_Http_Response_free           (struct OB_Http_Response*);
void                 OB_Http_Response_set_file       (struct OB_Http_Response*, FILE*);
bool                 OB_Http_Response_set_file_path  (struct OB_Http_Response*, const char*);
unsigned             OB_Http_Response_get_status_code(const struct OB_Http_Response*);
struct OB_Http_Body *OB_Http_Response_get_body       (struct OB_Http_Response*);

#endif