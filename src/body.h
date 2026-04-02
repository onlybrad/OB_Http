#ifndef OB_Body_H
#define OB_Body_H

#include <stdio.h>
#include <cjson.h>
#include "buffer.h"

enum OB_BodyType {
    OB_BODY_TYPE_NONE,
    OB_BODY_TYPE_BUFFER,
    OB_BODY_TYPE_FILE,
    OB_BODY_TYPE_URL_ENCODED,
    OB_BODY_TYPE_FORM_DATA,
    OB_BODY_TYPE_JSON,
    OB_BODY_TYPE_HTML,
    OB_BODY_TYPE_XML,
};

enum OB_JSON_Error {
    OB_JSON_ERROR_NONE,
    OB_JSON_ERROR_TOO_LARGE,
    OB_JSON_ERROR_PARSING,
};

struct OB_Body {
    enum OB_BodyType type;
    union {
        void            *none;   //for NONE
        struct OB_Buffer buffer; //for BUFFER
        FILE            *file;   //for FILE
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

void OB_Body_init(struct OB_Body*);

void              OB_Body_use_buffer(struct OB_Body*);
void              OB_Body_set_buffer(struct OB_Body*, struct OB_Buffer*);
bool              OB_Body_move_buffer(struct OB_Body*, struct OB_Buffer*);
struct OB_Buffer *OB_Body_get_buffer(struct OB_Body*);
struct OB_Buffer *OB_Body_get_html(struct OB_Body*);

void OB_Body_set_file     (struct OB_Body*, FILE*);
bool OB_Body_set_file_path(struct OB_Body*, const char*);
FILE *OB_Body_get_file    (struct OB_Body*);

void               OB_Body_use_json  (struct OB_Body*);
enum OB_JSON_Error OB_Body_parse_json(struct OB_Body*, struct OB_Buffer*);
struct CJSON      *OB_Body_get_json  (struct OB_Body*);

void OB_Body_free(struct OB_Body*);

#endif