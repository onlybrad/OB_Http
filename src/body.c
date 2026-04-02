#include <assert.h>
#include "body.h"

void OB_Body_init(struct OB_Body *const body) {
    assert(body != NULL);

    body->u.none = NULL;
    body->type   = OB_BODY_TYPE_NONE;
}

void OB_Body_use_buffer(struct OB_Body *const body) {
    assert(body != NULL);

    if(body->type != OB_BODY_TYPE_BUFFER) {
        OB_Body_free(body);
        body->type = OB_BODY_TYPE_BUFFER;
        OB_Buffer_init(&body->u.buffer, 0);
    }
}

void OB_Body_set_buffer(struct OB_Body *const body, struct OB_Buffer *const buffer) {
    assert(body != NULL);
    assert(buffer != NULL);

    OB_Body_free(body);
    body->type     = OB_BODY_TYPE_BUFFER;
    body->u.buffer = *buffer;
}

bool OB_Body_move_buffer(struct OB_Body *const body, struct OB_Buffer *const buffer) {
    assert(body != NULL);
    assert(buffer != NULL);

    if(body->type != OB_BODY_TYPE_BUFFER) {
        return false;
    }

    *buffer = body->u.buffer;
    OB_Body_init(body);
    return true;
}

struct OB_Buffer *OB_Body_get_buffer(struct OB_Body *const body) {
    assert(body != NULL);

    return body->type == OB_BODY_TYPE_BUFFER ? &body->u.buffer : NULL;
}

struct OB_Buffer *OB_Body_get_html(struct OB_Body *const body) {
    assert(body != NULL);

    return body->type == OB_BODY_TYPE_HTML ? &body->u.buffer : NULL;
}

void OB_Body_set_file(struct OB_Body *const body, FILE *const file) {
    assert(body != NULL);
    assert(file != NULL);

    OB_Body_free(body);
    body->type   = OB_BODY_TYPE_FILE;
    body->u.file = file;
}

bool OB_Body_set_file_path(struct OB_Body *const body, const char *file_path) {
    assert(body != NULL);
    assert(file_path != NULL);

    FILE *file = fopen(file_path, "rb");
    if(file == NULL) {
        return false;
    }

    OB_Body_set_file(body, file);

    return true;
}

FILE *OB_Body_get_file(struct OB_Body *const body) {
    assert(body != NULL);

    return body->type == OB_BODY_TYPE_FILE ? body->u.file : NULL;
}

void OB_Body_use_json(struct OB_Body *const body) {
    assert(body != NULL);

    if(body->type != OB_BODY_TYPE_JSON) {
        OB_Body_free(body);
        body->type = OB_BODY_TYPE_JSON;
        CJSON_Parser_init(&body->u.json.parser);
    }
}

enum OB_JSON_Error OB_Body_parse_json(struct OB_Body *const body, struct OB_Buffer *const buffer) {
    assert(body != NULL);
    assert(buffer != NULL);

    if(buffer->size > UINT_MAX) {
        return OB_JSON_ERROR_TOO_LARGE;
    }

    if(body->type != OB_BODY_TYPE_JSON) {
        OB_Body_use_json(body);
    }

    body->u.json.root = CJSON_parse(&body->u.json.parser, (const char*)buffer->data, (unsigned)buffer->size);
    
    return body->u.json.root == NULL
        ? OB_JSON_ERROR_PARSING
        : OB_JSON_ERROR_NONE;    
}

struct CJSON *OB_Body_get_json(struct OB_Body *const body) {
    assert(body != NULL);

    return body->type == OB_BODY_TYPE_JSON ? body->u.json.root : NULL;
}

void OB_Body_free(struct OB_Body *const body) {
    assert(body != NULL);

    switch(body->type) {
    case OB_BODY_TYPE_NONE:
        return;
    case OB_BODY_TYPE_BUFFER:
        OB_Buffer_free(&body->u.buffer);
        break;
    case OB_BODY_TYPE_FILE:
        fclose(body->u.file);
        break;
    case OB_BODY_TYPE_URL_ENCODED:
        assert(0 && "URL_ENCODED Unimplemented");
        break;
    case OB_BODY_TYPE_FORM_DATA:
        assert(0 && "FORM_DATA Unimplemented");
        break;
    case OB_BODY_TYPE_JSON:
        CJSON_Parser_free(&body->u.json.parser);
        break;
    case OB_BODY_TYPE_HTML:
        OB_Buffer_free(&body->u.buffer);
        break;
    case OB_BODY_TYPE_XML:
        assert(0 && "XML Unimplemented");
        break;     
    }

    OB_Body_init(body);
}