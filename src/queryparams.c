#include <assert.h>
#include <string.h>

#include "queryparams.h"
#include "util.h"

#define OB_Http_QueryParams_CAPACITY        8
#define OB_Http_QueryParams_MULTIPLY_FACTOR 2

void OB_Http_QueryParams_init(struct OB_Http_QueryParams *const query_params, CURL *const curl) {
    assert(query_params != NULL);
    assert(curl != NULL);

    query_params->capacity = 0;
    query_params->size     = 0;
    query_params->data     = NULL;
    query_params->curl     = curl;
    OB_Buffer_init(&query_params->string_buffer, 0);
}

void OB_Http_QueryParams_free(struct OB_Http_QueryParams *const query_params) {
    assert(query_params != NULL);

    for(size_t i = 0; i < query_params->size; i++) {
        struct OB_Http_QueryParam query_param = OB_Http_QueryParams_get(query_params, i);
        curl_free(query_param.encoded_name);
        curl_free(query_param.encoded_value);
    }

    OB_Buffer_free(&query_params->string_buffer);
    OB_Http_QueryParams_init(query_params, query_params->curl);
}

bool OB_Http_QueryParams_reserve(struct OB_Http_QueryParams *const query_params, const size_t capacity) {
    assert(query_params != NULL);
    assert(capacity > 0);

    if(query_params->capacity >= capacity) {
        return true;
    }

    const size_t size = capacity * sizeof(*query_params->data);
    struct OB_Http_IndexQueryParam *data;
    if((data = (struct OB_Http_IndexQueryParam*)OB_REALLOC(query_params->data, size)) == NULL) {
        return false;
    }

    query_params->data     = data;
    query_params->capacity = capacity;

    return true;
}

bool OB_Http_QueryParams_append(struct OB_Http_QueryParams *const query_params, const char *const name, const char *const value) {
    assert(query_params != NULL);
    assert(name != NULL);
    assert(value != NULL);

    if(query_params->size == query_params->capacity) {
        const size_t capacity = query_params->capacity == 0 
            ? OB_Http_QueryParams_CAPACITY 
            : OB_Http_QueryParams_MULTIPLY_FACTOR * query_params->capacity;

        if(!OB_Http_QueryParams_reserve(query_params, capacity)) {
            return false;
        }
    }

    struct OB_Http_IndexQueryParam *const query_param = query_params->data + query_params->size;
    query_param->encoded_name  = NULL;
    query_param->encoded_value = NULL;

    bool success = false;
    do {
        const unsigned char *bytes = (const unsigned char*)name;
        unsigned char *copied_name = OB_Buffer_append(&query_params->string_buffer, bytes, (size_t)-1);
        if(copied_name == NULL) {
            break;
        }

        query_param->name_index = (size_t)(copied_name - query_params->string_buffer.data);
        query_param->encoded_name = curl_easy_escape(query_params->curl, name, 0);
        if(query_param->encoded_name == NULL) {
            break;
        }

        bytes = (const unsigned char*)value;
        unsigned char *copied_value = OB_Buffer_append(&query_params->string_buffer, bytes, (size_t)-1);
        if(copied_value == NULL) {
            break;
        }
        query_param->value_index = (size_t)(copied_value - query_params->string_buffer.data);
        query_param->encoded_value = curl_easy_escape(query_params->curl, value, 0);
        
        success = query_param->encoded_value != NULL;
    } while(0);

    if(success) {
        query_params->size++;
        return true;
    } else {
        curl_free(query_param->encoded_name);
        curl_free(query_param->encoded_value);
        return false;
    }
}

bool OB_Http_QueryParams_set(struct OB_Http_QueryParams *const query_params, const char *const name, const char *const value) {
    assert(query_params != NULL);
    assert(name != NULL);
    assert(value != NULL);

    struct OB_Http_IndexQueryParam *query_param = NULL;
    for(size_t i = 0; i < query_params->size; i++) {
        const size_t name_index = query_params->data[i].name_index;
        const char *const name2 = (const char*)(query_params->string_buffer.data + name_index);
        if(strcmp(name, name2) == 0) {
            query_param = query_params->data + i;
            break;
        }
    }

    if(query_param == NULL) {
        return OB_Http_QueryParams_append(query_params, name, value);
    }

    char *encoded_value = NULL;

    bool success = false;
    do {
        char *const old_value = (char*)query_params->string_buffer.data + query_param->value_index;
        const size_t new_size = strlen(value);
        if(new_size >= (size_t)INT_MAX) {
            break;
        }

        encoded_value = curl_easy_escape(query_params->curl, value, (int)new_size);
        if(encoded_value == NULL) {
            break;
        }

        if(strlen(old_value) >= new_size) {
            strcpy(old_value, value);
            success = true;
            break;
        }
    
        const unsigned char *const u_value = (const unsigned char*)value;
        unsigned char *copied_value;
        if((copied_value = OB_Buffer_append(&query_params->string_buffer, u_value, new_size + 1)) == NULL) {
            break;
        }

        query_param->value_index = (size_t)(copied_value - query_params->string_buffer.data);

        success = true;      
    } while(0);

    if(success) {
        curl_free(query_param->encoded_value);
        query_param->encoded_value = encoded_value;
        return true;
    } else {
        curl_free(encoded_value);
        return false;
    }
}

struct OB_Http_QueryParam OB_Http_QueryParams_get(struct OB_Http_QueryParams *const query_params, const size_t index) {
    assert(query_params != NULL);
    assert(index < query_params->size);

    const struct OB_Http_IndexQueryParam *const index_query_param = query_params->data + index;

    const size_t name_index = index_query_param->name_index;
    const size_t value_index = index_query_param->value_index;

    struct OB_Http_QueryParam query_param;
    query_param.name = (char*)query_params->string_buffer.data + name_index;
    query_param.value = (char*)query_params->string_buffer.data + value_index;
    query_param.encoded_name = index_query_param->encoded_name;
    query_param.encoded_value = index_query_param->encoded_value;

    return query_param;
}

const char *OB_Http_QueryParams_get_value(struct OB_Http_QueryParams *const query_params, const char *const name) {
    assert(query_params != NULL);
    assert(name != NULL);

    for(size_t i = 0; i < query_params->size; i++) {
        const struct OB_Http_QueryParam query_param = OB_Http_QueryParams_get(query_params, i);
        if(strcmp(name, query_param.name) == 0) {
            return query_param.value;
        }
    }

    return NULL;
}