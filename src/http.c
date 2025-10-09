#include <assert.h>
#include "http.h"
#include "util.h"

#define OB_HEADERS_BUFFER_SIZE 1024      
#define OB_MAX_REDIRECTIONS    20

//macro used to call the curl_easy_setopt function and return OB_HTTP_ERROR_CURL if the function call fails
#define OB_CURL_SETOPT(CURL, CURLOPTION, VALUE)\
    do {\
        if((curl_code = curl_easy_setopt((CURL), (CURLOPTION), (VALUE))) != CURLE_OK) {\
            return OB_HTTP_ERROR_CURL;\
        }\
    } while(0)

//macro used to call the curl_easy_getinfo function and return OB_HTTP_ERROR_CURL if the function call fails
#define OB_CURL_GETINFO(CURL, CURLINFO, VALUE)\
    do {\
        if((curl_code = curl_easy_getinfo((CURL), (CURLINFO), (VALUE))) != CURLE_OK) {\
            return OB_HTTP_ERROR_CURL;\
        }\
    } while(0)

//macro used to call the curl_easy_perform function and return OB_HTTP_ERROR_CURL if the function call fails
#define OB_CURL_PERFORM(CURL)\
    do {\
        if((curl_code = curl_easy_perform((CURL))) != CURLE_OK) {\
            return OB_HTTP_ERROR_CURL;\
        }\
    } while(0)


static size_t OB_Http_write_callback(char *data, size_t size, size_t count, void *user_data) {
    assert(size == 1);
    assert(data != NULL);
    assert(user_data != NULL);

    struct OB_Buffer *const buffer = (struct OB_Buffer*)user_data;

    if(OB_Buffer_append(buffer, (unsigned char*)data, count) == NULL) {
        return CURL_WRITEFUNC_ERROR;
    }
    
    return count;
}


bool OB_HttpClient_init(struct OB_HttpClient *client) {
    assert(client != NULL);

    memset(client->error, 0, sizeof(client->error));

    client->max_redirections = OB_MAX_REDIRECTIONS;

    if(curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        return false;
    }

    return (client->curl = curl_easy_init()) != NULL;
}

void OB_HttpClient_free(struct OB_HttpClient *client) {
    assert(client != NULL);

    if(client->curl == NULL) {
        return;
    }

    curl_easy_cleanup(client->curl);
    curl_global_cleanup();
}

enum OB_HttpError OB_HttpClient_fetch(struct OB_HttpClient *client, struct OB_HttpRequest *request, struct OB_HttpResponse *response) {
    assert(client != NULL);
    assert(request != NULL);
    assert(response != NULL);

    static const char *const OB_CUSTOM_HTTP_METHODS[] = {
        /* OB_HTTP_METHOD_GET    */ NULL,
        /* OB_HTTP_METHOD_POST   */ NULL,
        /* OB_HTTP_METHOD_HEAD   */ NULL,
        /* OB_HTTP_METHOD_PUT    */ "PUT",
        /* OB_HTTP_METHOD_PATCH  */ "PATCH",
        /* OB_HTTP_METHOD_DELETE */ "DELETE"
    };

    CURLcode curl_code;

    OB_CURL_SETOPT(client->curl, CURLOPT_ERRORBUFFER, client->error);
    
    if(request->url == NULL) {
        return OB_HTTP_ERROR_MISSING_URL;
    }

    OB_CURL_SETOPT(client->curl, CURLOPT_URL, request->url);

    switch(request->method) {
    case OB_HTTP_METHOD_GET:
        OB_CURL_SETOPT(client->curl, CURLOPT_HTTPGET, 1L);
        break;

    case OB_HTTP_METHOD_POST:
        OB_CURL_SETOPT(client->curl, CURLOPT_POST, 1L);
        break;

    case OB_HTTP_METHOD_HEAD:
        OB_CURL_SETOPT(client->curl, CURLOPT_NOBODY, 1L);
        break;

    default:
        OB_CURL_SETOPT(client->curl, CURLOPT_CUSTOMREQUEST, OB_CUSTOM_HTTP_METHODS[(int)request->method]);
        break;
    }

    if(request->method != OB_HTTP_METHOD_HEAD) {
        OB_CURL_SETOPT(client->curl, CURLOPT_WRITEDATA, &response->body);
        OB_CURL_SETOPT(client->curl, CURLOPT_WRITEFUNCTION, OB_Http_write_callback);
        
        if(!OB_Buffer_reserve(&response->body, CURL_MAX_WRITE_SIZE)) {
            return OB_HTTP_ERROR_MALLOC;
        }
    }

    if(request->follow_redirections) {
        OB_CURL_SETOPT(client->curl, CURLOPT_FOLLOWLOCATION, CURLFOLLOW_ALL);
        OB_CURL_SETOPT(client->curl, CURLOPT_MAXREDIRS, (long)client->max_redirections);
    }

    OB_CURL_PERFORM(client->curl);

    long status_code;
    OB_CURL_GETINFO(client->curl, CURLINFO_RESPONSE_CODE, &status_code);
    response->status_code = (unsigned)status_code;

    struct curl_header *prev = NULL;
    struct curl_header *curr;
    while((curr = curl_easy_nextheader(client->curl, CURLH_HEADER, -1, prev)) != NULL) {
        response->header_count++;
        prev = curr;
    }
    prev = NULL;
    
    const size_t size = response->header_count * sizeof(*response->headers);
    if((response->headers = (struct OB_HttpHeader*)OB_MALLOC(size)) == NULL) {
        response->header_count = 0;
        return OB_HTTP_ERROR_MALLOC;
    }

    if(!OB_Buffer_reserve(&response->headers_buffer, OB_HEADERS_BUFFER_SIZE)) {
        return OB_HTTP_ERROR_MALLOC;
    }

    struct OB_HttpHeader *header = response->headers;
    while((curr = curl_easy_nextheader(client->curl, CURLH_HEADER, -1, prev)) != NULL) {
        unsigned char *const name      = (unsigned char*)curr->name;
        unsigned char *const value     = (unsigned char*)curr->value;
        struct OB_Buffer *const buffer = &response->headers_buffer;

        if((header->name = (char*)OB_Buffer_append(buffer, name, (size_t)-1)) == NULL) {
            return OB_HTTP_ERROR_MALLOC;
        }
        if((header->value = (char*)OB_Buffer_append(buffer, value, (size_t)-1)) == NULL) {
            return OB_HTTP_ERROR_MALLOC;
        }
        header++;
        prev = curr;
    }

    return OB_HTTP_ERROR_NONE;
}

void OB_HttpRequest_init(struct OB_HttpRequest *request) {
    assert(request != NULL);
    
    request->url                 = NULL;
    request->method              = OB_HTTP_METHOD_GET;
    request->follow_redirections = true;
}

void OB_HttpResponse_init(struct OB_HttpResponse *response) {
    assert(response != NULL);

    OB_Buffer_init(&response->body, 0);
    OB_Buffer_init(&response->headers_buffer, 0);
    response->headers       = NULL;
    response->header_count  = 0;
    response->status_code   = 0;
}

void OB_HttpResponse_free(struct OB_HttpResponse *response) {
    assert(response != NULL);

    OB_Buffer_free(&response->body);
    OB_Buffer_free(&response->headers_buffer);
    OB_FREE(response->headers);
    response->headers       = NULL;
    response->header_count  = 0;
    response->status_code   = 0;
}