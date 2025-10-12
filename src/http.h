#ifndef OB_HTTP_H
#define OB_HTTP_H

#include <curl\curl.h>
#include <stdbool.h>
#include "error.h"
#include "buffer.h"
#include "header.h"

enum OB_Http_Method {
    OB_HTTP_METHOD_GET,
    OB_HTTP_METHOD_POST,
    OB_HTTP_METHOD_HEAD,
    OB_HTTP_METHOD_PUT,
    OB_HTTP_METHOD_PATCH,
    OB_HTTP_METHOD_DELETE
};

enum OB_Http_BodyType {
    OB_HTTP_BODY_TYPE_NONE,
    OB_HTTP_BODY_TYPE_BYTES,
    OB_HTTP_BODY_TYPE_BUFFER,
    OB_HTTP_BODY_TYPE_FILE,
    OB_HTTP_BODY_TYPE_URL_ENCODED,
    OB_HTTP_BODY_TYPE_FORM_DATA,
    OB_HTTP_BODY_TYPE_JSON,
    OB_HTTP_BODY_TYPE_XML,
};

struct OB_Http_Body {
    void                 *value;
    enum OB_Http_BodyType type;
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
    enum OB_Http_Method    method;
    struct OB_Http_Headers headers;
    bool                   follow_redirections;
    struct curl_slist     *curl_headers;
};

struct OB_Http_Response {
    struct OB_Buffer       body;
    struct OB_Http_Headers headers;
    unsigned               status_code;
};

bool               OB_Http_Client_init(struct OB_Http_Client*);
void               OB_Http_Client_free(struct OB_Http_Client*);
enum OB_Http_Error OB_Http_Client_fetch(struct OB_Http_Client*, struct OB_Http_Request*, struct OB_Http_Response*);
bool               OB_Http_Client_get_headers(struct OB_Http_Client*, struct OB_Http_Response*);

void OB_Http_Request_init(struct OB_Http_Request*);
void OB_Http_Request_free(struct OB_Http_Request*);

void OB_Http_Response_init(struct OB_Http_Response*);
void OB_Http_Response_free(struct OB_Http_Response*);

#endif