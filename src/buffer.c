#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "buffer.h"
#include "util.h"

#define OB_BUFFER_MULTIPLY_FACTOR 2

bool OB_Buffer_init(struct OB_Buffer *buffer, size_t capacity) {
    assert(buffer != NULL);
    
    buffer->data     = NULL;
    buffer->size     = 0;
    buffer->capacity = 0;

    if(capacity == 0) {
        return true;
    }

    if((buffer->data = (unsigned char*)OB_MALLOC(capacity)) != NULL) {
        buffer->capacity = capacity;
        return true;
    }

    return false;
}

bool OB_Buffer_reserve(struct OB_Buffer *buffer, size_t capacity) {
    assert(buffer != NULL);
    assert(capacity > 0);

    if(buffer->capacity >= capacity) {
        return true;
    }
    
    unsigned char *data;
    if((data = (unsigned char*)OB_REALLOC(buffer->data, capacity)) == NULL) {
        return false;
    }

    buffer->data     = data;
    buffer->capacity = capacity;

    return true;
}

unsigned char *OB_Buffer_append(struct OB_Buffer *buffer, const unsigned char *data, size_t size) {
    assert(buffer != NULL);
    assert(data != NULL);
    assert(size > 0);

    if(size == (size_t)-1) {
        size = strlen((const char*)data) + 1;
    }

    const size_t new_size = buffer->size + size;

    if(new_size > buffer->capacity) {
        const size_t new_capacity = OB_BUFFER_MULTIPLY_FACTOR * new_size;

        unsigned char *new_data;
        if((new_data = (unsigned char*)OB_REALLOC(buffer->data, new_capacity)) == NULL) {
            return NULL;
        }

        buffer->data     = new_data;
        buffer->capacity = new_capacity;
    }
    
    unsigned char *const ret = buffer->data + buffer->size;
    memcpy(ret, data, size);
    buffer->size = new_size;

    return ret;
}

void OB_Buffer_reset(struct OB_Buffer *buffer) {
    assert(buffer != NULL);

    buffer->size = 0;
}

void OB_Buffer_free(struct OB_Buffer *buffer) {
    assert(buffer != NULL);

    OB_FREE(buffer->data);
    buffer->data     = NULL;
    buffer->size     = 0;
    buffer->capacity = 0;
}