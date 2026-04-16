#ifndef OB_HTTP_QUERY_PARAMS_H
#define OB_HTTP_QUERY_PARAMS_H

#include <stdint.h>
#include <curl/curl.h>

#include "buffer.h"

struct OB_Http_QueryParam {
    char *name;
    char *value;
    char *encoded_name;
    char *encoded_value;
};

struct OB_Http_IndexQueryParam {
    size_t name_index;
    size_t value_index;
    char  *encoded_name;
    char  *encoded_value;
};

struct OB_Http_QueryParams {
    struct OB_Buffer                string_buffer;
    struct OB_Http_IndexQueryParam *data;
    CURL                           *curl;
    size_t                          capacity;
    size_t                          size;
};

void OB_Http_QueryParams_init(struct OB_Http_QueryParams*, CURL *curl);
void OB_Http_QueryParams_free(struct OB_Http_QueryParams*);
bool OB_Http_QueryParams_reserve(struct OB_Http_QueryParams*, size_t);
bool OB_Http_QueryParams_append(struct OB_Http_QueryParams*, const char *name, const char *value);
bool OB_Http_QueryParams_set(struct OB_Http_QueryParams*, const char *name, const char *value);
struct OB_Http_QueryParam OB_Http_QueryParams_get(struct OB_Http_QueryParams*, size_t);
const char *OB_Http_QueryParams_get_value(struct OB_Http_QueryParams*, const char*);

#endif