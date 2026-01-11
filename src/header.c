#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "header.h"
#include "util.h"

#define OB_HTTP_HEADERS_CAPACITY        8
#define OB_HTTP_HEADERS_MULTIPLY_FACTOR 2

void OB_Http_Headers_init(struct OB_Http_Headers *headers) {
    assert(headers != NULL);

    headers->capacity = 0;
    headers->size     = 0;
    headers->data     = NULL;
    OB_Buffer_init(&headers->string_buffer, 0);
}

void OB_Http_Headers_free(struct OB_Http_Headers *headers) {
    assert(headers != NULL);

    OB_Buffer_free(&headers->string_buffer);
    OB_Http_Headers_init(headers);
}

bool OB_Http_Headers_reserve(struct OB_Http_Headers *headers, size_t capacity) {
    assert(headers != NULL);
    assert(capacity > 0);

    if(headers->capacity >= capacity) {
        return true;
    }

    const size_t size = capacity * sizeof(*headers->data);
    struct OB_Http_IndexHeader *data;
    if((data = (struct OB_Http_IndexHeader*)OB_REALLOC(headers->data, size)) == NULL) {
        return false;
    }

    headers->data     = data;
    headers->capacity = capacity;

    return true;
}

bool OB_Http_Headers_append(struct OB_Http_Headers *headers, const char *name, const char *value) {
    assert(headers != NULL);
    assert(name != NULL);
    assert(value != NULL);

    if(headers->size == headers->capacity) {
        const size_t capacity = headers->capacity == 0 
            ? OB_HTTP_HEADERS_CAPACITY 
            : OB_HTTP_HEADERS_MULTIPLY_FACTOR * headers->capacity;

        if(!OB_Http_Headers_reserve(headers, capacity)) {
            return false;
        }
    }

    struct OB_Http_IndexHeader *const header = headers->data + headers->size;

    const unsigned char *bytes = (const unsigned char*)name;
    unsigned char *copied_name;
    if((copied_name = OB_Buffer_append(&headers->string_buffer, bytes, (size_t)-1)) == NULL) {
        return false;
    }
    OB_Util_strtolower((char*)copied_name);
    header->name_index  = (size_t)(copied_name - headers->string_buffer.data);

    bytes = (const unsigned char*)value;
    unsigned char *copied_value;
    if((copied_value = OB_Buffer_append(&headers->string_buffer, bytes, (size_t)-1)) == NULL) {
        return false;
    }
    header->value_index = (size_t)(copied_value - headers->string_buffer.data);

    headers->size++;

    return true;
}

bool OB_Http_Headers_set(struct OB_Http_Headers *headers, const char *name, const char *value) {
    assert(headers != NULL);
    assert(name != NULL);
    assert(value != NULL);

    struct OB_Http_IndexHeader *header = NULL;
    for(size_t i = 0; i < headers->size; i++) {
        const size_t name_index = headers->data[i].name_index;
        const char *const name2 = (const char*)(headers->string_buffer.data + name_index);
        if(strcmp(name, name2) == 0) {
            header = headers->data + i;
            break;
        }
    }

    if(header == NULL) {
        return OB_Http_Headers_append(headers, name, value);
    }

    char *const old_value = (char*)headers->string_buffer.data + header->value_index;
    const size_t new_size = strlen(value);
    if(strlen(old_value) >= new_size) {
        strcpy(old_value, value);
        return true;
    } 
    
    const unsigned char *const u_value = (const unsigned char*)value;
    unsigned char *copied_value;
    if((copied_value = OB_Buffer_append(&headers->string_buffer, u_value, new_size + 1)) == NULL) {
        return false;
    }

    header->value_index = (size_t)(copied_value - headers->string_buffer.data);

    return true;
}

struct OB_Http_Header OB_Http_Headers_get(struct OB_Http_Headers *headers, size_t index) {
    assert(headers != NULL);
    assert(index < headers->size);

    const size_t name_index = headers->data[index].name_index;
    const size_t value_index = headers->data[index].value_index;

    struct OB_Http_Header header = {
        (char*)headers->string_buffer.data + name_index,
        (char*)headers->string_buffer.data + value_index
    };

    return header;
}

const char *OB_Http_Headers_get_value(struct OB_Http_Headers *headers, const char *name) {
    assert(headers != NULL);
    assert(name != NULL);

    for(size_t i = 0; i < headers->size; i++) {
        const struct OB_Http_Header header = OB_Http_Headers_get(headers, i);
        if(strcmp(name, header.name) == 0) {
            return header.value;
        }
    }

    return NULL;
}