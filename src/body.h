#ifndef OB_HTTP_BODY_H
#define OB_HTTP_BODY_H

#include <stdio.h>
#include <cjson.h>
#include "buffer.h"

enum OB_Http_BodyType {
    OB_HTTP_BODY_TYPE_NONE,
    OB_HTTP_BODY_TYPE_BUFFER,
    OB_HTTP_BODY_TYPE_FILE,
    OB_HTTP_BODY_TYPE_URL_ENCODED,
    OB_HTTP_BODY_TYPE_FORM_DATA,
    OB_HTTP_BODY_TYPE_JSON,
    OB_HTTP_BODY_TYPE_HTML,
    OB_HTTP_BODY_TYPE_XML,
};

struct OB_Http_Body {
    union {
        void               *none;   //for NONE
        struct OB_Buffer    buffer; //for BUFFER
        FILE               *file;   //for FILE
        //reserved for URL_ENCODED
        //reserved for FORM_DATA
        struct CJSON_Parser json_parser; //for JSON
        //reserved got HTML
        //reserved for XML
    } u;
    enum OB_Http_BodyType type;
};

void OB_Http_Body_init(struct OB_Http_Body*);
void OB_Http_Body_move_file(struct OB_Http_Body*, FILE*);
void OB_Http_Body_move_bytes(struct OB_Http_Body*, unsigned char *bytes, size_t size);
void OB_Http_Body_move_json(struct OB_Http_Body*, struct CJSON_Parser*);
void OB_Http_Body_move_buffer(struct OB_Http_Body*, struct OB_Buffer*);
void OB_Http_Body_use_buffer(struct OB_Http_Body*);
bool OB_Http_Body_set_file_path(struct OB_Http_Body*, const char*);


void OB_Http_Body_free(struct OB_Http_Body*);

#endif