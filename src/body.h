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

enum OB_JSON_Error {
    OB_JSON_ERROR_NONE,
    OB_JSON_ERROR_TOO_LARGE,
    OB_JSON_ERROR_PARSING,
};

struct OB_Http_Body {
    enum OB_Http_BodyType type;
    union {
        void            *none;   //for NONE
        FILE            *file;
        struct OB_Buffer buffer; //for BUFFER
        //reserved for URL_ENCODED
        //reserved for FORM_DATA
        struct {  //for JSON
            struct CJSON_Parser parser;
            struct CJSON       *root;
        } json;
        //reserved got HTML
        //reserved for XML
    } u;
};

void OB_Http_Body_init(struct OB_Http_Body*);

void              OB_Http_Body_use_buffer (struct OB_Http_Body*);
void              OB_Http_Body_set_buffer (struct OB_Http_Body*, struct OB_Buffer*);
bool              OB_Http_Body_move_buffer(struct OB_Http_Body*, struct OB_Buffer*);
struct OB_Buffer *OB_Http_Body_get_buffer (struct OB_Http_Body*);
struct OB_Buffer *OB_Http_Body_get_html   (struct OB_Http_Body*);

void               OB_Http_Body_use_json  (struct OB_Http_Body*);
enum OB_JSON_Error OB_Http_Body_parse_json(struct OB_Http_Body*, struct OB_Buffer*);
struct CJSON      *OB_Http_Body_get_json  (struct OB_Http_Body*);

void              OB_Http_Body_set_file   (struct OB_Http_Body*, FILE *file);
bool              OB_Http_Body_set_file_path(struct OB_Http_Body*, const char *path, bool is_readmode);

void OB_Http_Body_free(struct OB_Http_Body*);

#endif