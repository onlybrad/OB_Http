#ifndef OB_HTTP_HEADER_H
#define OB_HTTP_HEADER_H

#include <stdint.h>
#include "buffer.h"

enum OB_Http_ContentType {
    OB_HTTP_CONTENT_TYPE_TEXT,
    OB_HTTP_CONTENT_TYPE_HTML,
    OB_HTTP_CONTENT_TYPE_JSON,
    OB_HTTP_CONTENT_TYPE_XML,
    OB_HTTP_CONTENT_TYPE_URLENCODED,
    OB_HTTP_CONTENT_TYPE_FORMDATA,
    OB_HTTP_CONTENT_TYPE_OCTET_STREAM,
    OB_HTTP_CONTENT_TYPE_CUSTOM
};

struct OB_Http_Header {
    char *name;
    char *value;
};

struct OB_Http_IndexHeader {
    size_t name_index;
    size_t value_index;
};

struct OB_Http_Headers {
    struct OB_Buffer            string_buffer;
    struct OB_Http_IndexHeader *data;
    size_t                      capacity;
    size_t                      size;
};

void OB_Http_Headers_init(struct OB_Http_Headers*);
void OB_Http_Headers_free(struct OB_Http_Headers*);
bool OB_Http_Headers_reserve(struct OB_Http_Headers*, size_t);
bool OB_Http_Headers_append(struct OB_Http_Headers*, const char *name, const char *value);
bool OB_Http_Headers_set(struct OB_Http_Headers*, const char *name, const char *value);
bool OB_Http_Headers_set_accept(struct OB_Http_Headers*, enum OB_Http_ContentType, const char *custom);
bool OB_Http_Headers_set_content_type(struct OB_Http_Headers*, enum OB_Http_ContentType, const char *custom);
bool OB_Http_Headers_set_authorization(struct OB_Http_Headers*, const char*);
struct OB_Http_Header OB_Http_Headers_get(struct OB_Http_Headers*, size_t);

#endif