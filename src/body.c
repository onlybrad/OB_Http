#include <assert.h>
#include "body.h"

void OB_Http_Body_init(struct OB_Http_Body *body) {
    assert(body != NULL);

    body->u.none = NULL;
    body->type   = OB_HTTP_BODY_TYPE_NONE;
}

void OB_Http_Body_move_file(struct OB_Http_Body *body, FILE *file) {
    assert(body != NULL);
    assert(file != NULL);

    OB_Http_Body_free(body);
    body->u.file = file;
    body->type = OB_HTTP_BODY_TYPE_FILE;
}

void OB_Http_Body_move_bytes(struct OB_Http_Body *body, unsigned char *bytes, size_t size) {
    assert(body != NULL);

    OB_Http_Body_free(body);
    struct OB_Buffer *const buffer = &body->u.buffer;
    buffer->size     = size;
    buffer->capacity = size;
    buffer->data     = bytes;
}

void OB_Http_Body_move_json(struct OB_Http_Body *body, struct CJSON_Parser *parser) {
    assert(body != NULL);
    assert(parser != NULL);

    OB_Http_Body_free(body);
    body->u.json_parser = *parser;
    body->type          = OB_HTTP_BODY_TYPE_JSON;
}

void OB_Http_Body_move_buffer(struct OB_Http_Body *body, struct OB_Buffer *buffer) {
    assert(body != NULL);
    assert(buffer != NULL); 

    OB_Http_Body_free(body);
    body->u.buffer = *buffer;
    body->type     = OB_HTTP_BODY_TYPE_BUFFER;
    OB_Buffer_init(buffer, 0);
}

void OB_Http_Body_use_buffer(struct OB_Http_Body *body) {
    assert(body != NULL);

    OB_Http_Body_free(body);
    OB_Buffer_init(&body->u.buffer, 0);
    body->type = OB_HTTP_BODY_TYPE_BUFFER;
}

bool OB_Http_Body_set_file_path(struct OB_Http_Body *body, const char *file_path) {
    assert(body != NULL);
    assert(file_path != NULL);

    OB_Http_Body_free(body);
    //TODO handle utf8 path names on windows
    if((body->u.file = fopen(file_path, "rb")) == NULL) {
        return false;
    }

    body->type = OB_HTTP_BODY_TYPE_FILE;

    return true;
}

void OB_Http_Body_free(struct OB_Http_Body *body) {
    assert(body != NULL);

    switch(body->type) {
    case OB_HTTP_BODY_TYPE_NONE:
        return;
    case OB_HTTP_BODY_TYPE_BUFFER:
        OB_Buffer_free(&body->u.buffer);
        break;
    case OB_HTTP_BODY_TYPE_FILE:
        fclose(body->u.file);
        break;
    case OB_HTTP_BODY_TYPE_URL_ENCODED:
        assert(0 && "URL_ENCODED Unimplemented");
        break;
    case OB_HTTP_BODY_TYPE_FORM_DATA:
        assert(0 && "FORM_DATA Unimplemented");
        break;
    case OB_HTTP_BODY_TYPE_JSON:
        CJSON_Parser_free(&body->u.json_parser);
        break;
    case OB_HTTP_BODY_TYPE_HTML:
        assert(0 && "HTML Unimplemented");
        break;     
    case OB_HTTP_BODY_TYPE_XML:
        assert(0 && "XML Unimplemented");
        break;     
    }

    OB_Http_Body_init(body);
}