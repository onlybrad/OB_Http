#ifndef OB_HTTP_H
#define OB_HTTP_H

#include <curl\curl.h>
#include <stdbool.h>
#include "error.h"
#include "buffer.h"

enum OB_HttpMethod {
    OB_HTTP_METHOD_GET,
    OB_HTTP_METHOD_POST,
    OB_HTTP_METHOD_HEAD,
    OB_HTTP_METHOD_PUT,
    OB_HTTP_METHOD_PATCH,
    OB_HTTP_METHOD_DELETE
};

struct OB_HttpHeader {
    char *name;
    char *value;
};

struct OB_HttpClient {
    CURL    *curl;
    unsigned max_redirections;
    char     error[CURL_ERROR_SIZE];
};

struct OB_HttpRequest {
    const char        *url;
    enum OB_HttpMethod method;
    bool               follow_redirections;
};

struct OB_HttpResponse {
    struct OB_Buffer body;
    struct OB_Buffer headers_buffer;
    struct OB_HttpHeader *headers;
    size_t header_count;
    unsigned status_code;
};

bool              OB_HttpClient_init(struct OB_HttpClient*);
void              OB_HttpClient_free(struct OB_HttpClient*);
enum OB_HttpError OB_HttpClient_fetch(struct OB_HttpClient*, struct OB_HttpRequest*, struct OB_HttpResponse*);

void OB_HttpRequest_init(struct OB_HttpRequest*);

void OB_HttpResponse_init(struct OB_HttpResponse*);
void OB_HttpResponse_free(struct OB_HttpResponse*);

#endif