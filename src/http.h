#ifndef OB_HTTP_H
#define OB_HTTP_H

#include <curl/curl.h>
#include <stdbool.h>

#include "error.h"
#include "buffer.h"
#include "body.h"
#include "header.h"

enum OB_Http_Method {
    OB_HTTP_METHOD_GET,
    OB_HTTP_METHOD_POST,
    OB_HTTP_METHOD_HEAD,
    OB_HTTP_METHOD_PUT,
    OB_HTTP_METHOD_PATCH,
    OB_HTTP_METHOD_DELETE
};

enum OB_BodySource_Type {
    OB_BODY_SOURCE_TYPE_BUFFER,
    OB_BODY_SOURCE_TYPE_FILE
};

union OB_BodySource_Value {
    FILE*          file;
    struct OB_Body body;
};

struct OB_BodySource {
    enum OB_BodySource_Type type;
    union OB_BodySource_Value value;
};

struct OB_Http_Client {
    CURL    *curl;
    unsigned max_redirections;
    bool     get_headers;
    bool     get_body;
    char     error[CURL_ERROR_SIZE];
};

struct OB_Http_Request {
    const char            *url;
    struct OB_BodySource   body_source;
    enum OB_Http_Method    method;
    struct OB_Http_Headers headers;
    bool                   follow_redirections;
    struct curl_slist     *curl_headers;
};

struct OB_Http_Response {
    struct OB_BodySource   body_source;
    struct OB_Http_Headers headers;
    unsigned               status_code;
};

bool               OB_Http_Client_init(struct OB_Http_Client*);
void               OB_Http_Client_free(struct OB_Http_Client*);
enum OB_Http_Error OB_Http_Client_fetch(struct OB_Http_Client*, struct OB_Http_Request*, struct OB_Http_Response*);
bool               OB_Http_Client_get_headers(struct OB_Http_Client*, struct OB_Http_Response*);
const char        *OB_Http_Client_get_error(const struct OB_Http_Client*);

void OB_Http_Request_init(struct OB_Http_Request*);
void OB_Http_Request_free(struct OB_Http_Request*);
void OB_Http_Request_set_url(struct OB_Http_Request*, const char*);
bool OB_Http_Request_basic_auth(struct OB_Http_Request*, const char *username, const char *password);
void OB_Http_Request_set_file(struct OB_Http_Request*, FILE*);
bool OB_Http_Request_set_file_path(struct OB_Http_Request*, const char*);

void OB_Http_Response_init(struct OB_Http_Response*);
void OB_Http_Response_free(struct OB_Http_Response*);
void OB_Http_Response_set_file(struct OB_Http_Response*, FILE*);
bool OB_Http_Response_set_file_path(struct OB_Http_Response*, const char*);
unsigned OB_Http_Response_get_status_code(const struct OB_Http_Response*);
struct OB_Body *OB_Http_Response_get_body(struct OB_Http_Response*);


#endif