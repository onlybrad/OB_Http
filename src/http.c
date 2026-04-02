#include <assert.h>
#include "http.h"
#include "util.h"

#define OB_HEADERS_BUFFER_SIZE 1024 
#define OB_MAX_HEADER_SIZE     (1 << 13)
#define OB_MAX_REDIRECTIONS    20

//macro used to call the curl_easy_setopt function and return OB_HTTP_ERROR_CURL if the function call fails
#define OB_CURL_SETOPT(CURL, CURLOPTION, VALUE)\
    do {\
        if(curl_easy_setopt((CURL), (CURLOPTION), (VALUE)) != CURLE_OK) {\
            return OB_HTTP_ERROR_CURL;\
        }\
    } while(0)

#define OB_CURL_SETOPT_OR_BREAK(CURL, CURLOPTION, VALUE)\
    do {\
        if(curl_easy_setopt((CURL), (CURLOPTION), (VALUE)) != CURLE_OK) {\
            error = OB_HTTP_ERROR_CURL;\
            break;\
        }\
    } while(0)

//macro used to call the curl_easy_getinfo function and return OB_HTTP_ERROR_CURL if the function call fails
#define OB_CURL_GETINFO(CURL, CURLINFO, VALUE)\
    do {\
        if(curl_easy_getinfo((CURL), (CURLINFO), (VALUE)) != CURLE_OK) {\
            return OB_HTTP_ERROR_CURL;\
        }\
    } while(0)

//macro used to call the curl_easy_perform function and return OB_HTTP_ERROR_CURL if the function call fails
#define OB_CURL_PERFORM(CURL)\
    do {\
        if(curl_easy_perform((CURL)) != CURLE_OK) {\
            return OB_HTTP_ERROR_CURL;\
        }\
    } while(0)


static size_t OB_Http_Client_write_callback(char *data, size_t size, size_t count, void *user_data) {
    assert(size == 1);
    assert(data != NULL);
    assert(user_data != NULL);

    struct OB_Buffer *const buffer = (struct OB_Buffer*)user_data;

    if(OB_Buffer_append(buffer, (unsigned char*)data, count) == NULL) {
        return CURL_WRITEFUNC_ERROR;
    }
    
    return count;
}

static size_t OB_Http_Client_noop_callback(char *data, size_t size, size_t count, void *user_data) {
    assert(size == 1);
    assert(data != NULL);

    (void)data;
    (void)user_data;
    (void)size;

    return count;
}

static enum OB_Http_Error OB_Http_Client_prepare_request_headers(struct OB_Http_Client *client, struct OB_Http_Request *request) {
    assert(client != NULL);
    assert(request != NULL);

    enum OB_Http_Error error = OB_HTTP_ERROR_NONE;

    for(size_t i = 0; i < request->headers.size; i++) {
        struct OB_Http_Header header = OB_Http_Headers_get(&request->headers, i);

        //using CURLOPT_ACCEPT_ENCODING instead of CURLOPT_HTTPHEADER will ensure that decoding is done automatically by curl
        if(strcmp(header.name, "accept-encoding") == 0) {
            OB_CURL_SETOPT_OR_BREAK(client->curl, CURLOPT_ACCEPT_ENCODING, header.value);
            continue;
        }

        if(strcmp(header.name, "authorization") == 0) {
            static const char basic[] = "Basic "; 
            if(strncmp(header.value, basic, sizeof(basic) - 1) == 0 && strlen(header.value) > sizeof(basic) - 1) {
                const char *const credentials = header.value + sizeof(basic) - 1;

                OB_CURL_SETOPT_OR_BREAK(client->curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
                OB_CURL_SETOPT_OR_BREAK(client->curl, CURLOPT_USERPWD, credentials);
                continue;
            }
        }

        char str_header[OB_MAX_HEADER_SIZE];
        if((size_t)snprintf(str_header, sizeof(str_header), "%s: %s", header.name, header.value) >= sizeof(str_header)) {
            error = OB_HTTP_ERROR_HEADER_SIZE;
            break;
        }

        if((request->curl_headers = curl_slist_append(request->curl_headers, str_header)) == NULL) {
            error = OB_HTTP_ERROR_MALLOC;
            break;
        }
    }

    if((curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, request->curl_headers)) != CURLE_OK) {
        error = OB_HTTP_ERROR_CURL;
    }

    if(error != OB_HTTP_ERROR_NONE) {
        curl_slist_free_all(request->curl_headers);
        request->curl_headers = NULL;
    }

    return error;
}

static enum OB_Http_Error OB_Http_Client_decode_body(struct OB_Http_Client *client, struct OB_Http_Response *response) {
    assert(client != NULL);
    assert(response != NULL);

    struct curl_header *content_type;
    if(curl_easy_header(client->curl, "content-type", 0, CURLH_HEADER, -1, &content_type) != CURLHE_OK) {
        return OB_HTTP_ERROR_CURL;
    }

    static const char text_plain[] = "text/plain";
    if(strncmp(content_type->value, text_plain, sizeof(text_plain) - 1) == 0) {
        response->body.type = OB_BODY_TYPE_BUFFER;
        return OB_HTTP_ERROR_NONE;
    }

    static const char application_json[] = "application/json";
    if(strncmp(content_type->value, application_json, sizeof(application_json) - 1) == 0) {
        struct OB_Buffer buffer;
        const bool success = OB_Body_move_buffer(&response->body, &buffer);
        assert(success);
        (void)success;

        switch(OB_Body_parse_json(&response->body, &buffer)) {
        case OB_JSON_ERROR_NONE:
            OB_Buffer_free(&buffer);
            return OB_HTTP_ERROR_NONE;
        
        case OB_JSON_ERROR_TOO_LARGE:
            OB_Body_set_buffer(&response->body, &buffer);
            return OB_HTTP_ERROR_TOO_LARGE;
        
        case OB_JSON_ERROR_PARSING:
            OB_Body_set_buffer(&response->body, &buffer);
            return OB_HTTP_ERROR_JSON_PARSING;
        }
    }

    static const char text_html[] = "text/html";
    if(strncmp(content_type->value, text_html, sizeof(text_html) - 1) == 0) {
        response->body.type = OB_BODY_TYPE_HTML;
        return OB_HTTP_ERROR_NONE;
    }

    static const char application_xml[] = "application/xml";
    if(strncmp(content_type->value, application_xml, sizeof(application_xml) - 1) == 0) {
        response->body.type = OB_BODY_TYPE_XML;
        return OB_HTTP_ERROR_NONE;
    }

    static const char urlencoded[] = "application/x-www-form-urlencoded";
    if(strncmp(content_type->value, urlencoded, sizeof(urlencoded) - 1) == 0) {
        response->body.type = OB_BODY_TYPE_URL_ENCODED;
        return OB_HTTP_ERROR_NONE;
    }

    static const char multipart_form_data[] = "multipart/form-data";
    if(strncmp(content_type->value, multipart_form_data, sizeof(multipart_form_data) - 1) == 0) {
        response->body.type = OB_BODY_TYPE_FORM_DATA;
        return OB_HTTP_ERROR_NONE;
    }
    
    static const char octect_stream[] = "application/octet-stream";
    if(strncmp(content_type->value, octect_stream, sizeof(octect_stream) - 1) == 0) {
        response->body.type = OB_BODY_TYPE_BUFFER;
        return OB_HTTP_ERROR_NONE;
    }

    response->body.type = OB_BODY_TYPE_BUFFER;
    return OB_HTTP_ERROR_NONE;
}


bool OB_Http_Client_init(struct OB_Http_Client *client) {
    assert(client != NULL);

    memset(client->error, 0, sizeof(client->error));

    client->max_redirections = OB_MAX_REDIRECTIONS;
    client->get_headers      = false;
    client->get_body         = true;

    if(curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        return false;
    }

    return (client->curl = curl_easy_init()) != NULL;
}

void OB_Http_Client_free(struct OB_Http_Client *client) {
    assert(client != NULL);

    if(client->curl == NULL) {
        return;
    }

    curl_easy_cleanup(client->curl);
    curl_global_cleanup();
}

enum OB_Http_Error OB_Http_Client_fetch(struct OB_Http_Client *client, struct OB_Http_Request *request, struct OB_Http_Response *response) {
    assert(client != NULL);
    assert(request != NULL);
    assert(response != NULL);

    enum OB_Http_Error error;

    static const char *const OB_HTTP_METHODS[] = {
        /* OB_HTTP_METHOD_GET    */ NULL,
        /* OB_HTTP_METHOD_POST   */ NULL,
        /* OB_HTTP_METHOD_HEAD   */ NULL,
        /* OB_HTTP_METHOD_PUT    */ "PUT",
        /* OB_HTTP_METHOD_PATCH  */ "PATCH",
        /* OB_HTTP_METHOD_DELETE */ "DELETE"
    };

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
        OB_CURL_SETOPT(client->curl, CURLOPT_CUSTOMREQUEST, OB_HTTP_METHODS[(int)request->method]);
        break;
    }

    if(client->get_body && request->method != OB_HTTP_METHOD_HEAD) {
        OB_CURL_SETOPT(client->curl, CURLOPT_WRITEDATA, &response->body.u.buffer);
        OB_CURL_SETOPT(client->curl, CURLOPT_WRITEFUNCTION, OB_Http_Client_write_callback);
        
        OB_Body_use_buffer(&response->body);
        if(!OB_Buffer_reserve(&response->body.u.buffer, CURL_MAX_WRITE_SIZE)) {
            return OB_HTTP_ERROR_MALLOC;
        }
    } else {
        OB_CURL_SETOPT(client->curl, CURLOPT_WRITEDATA, NULL);
        OB_CURL_SETOPT(client->curl, CURLOPT_WRITEFUNCTION, OB_Http_Client_noop_callback);
    }

    if(request->follow_redirections) {
        OB_CURL_SETOPT(client->curl, CURLOPT_FOLLOWLOCATION, CURLFOLLOW_ALL);
        OB_CURL_SETOPT(client->curl, CURLOPT_MAXREDIRS, (long)client->max_redirections);
    } else {
        OB_CURL_SETOPT(client->curl, CURLOPT_FOLLOWLOCATION, 0L);
        OB_CURL_SETOPT(client->curl, CURLOPT_MAXREDIRS, 0L);
    }

    if(request->headers.size > 0) {
        if((error = OB_Http_Client_prepare_request_headers(client, request)) != OB_HTTP_ERROR_NONE) {
            return error;
        }
    }

    OB_CURL_PERFORM(client->curl);

    long status_code;
    OB_CURL_GETINFO(client->curl, CURLINFO_RESPONSE_CODE, &status_code);
    response->status_code = (unsigned)status_code;

    if(client->get_headers && !OB_Http_Client_get_headers(client, response)) {
        return OB_HTTP_ERROR_MALLOC;   
    }

    if(client->get_body && (error = OB_Http_Client_decode_body(client, response)) != OB_HTTP_ERROR_NONE) {
        return error;
    }
    
    return OB_HTTP_ERROR_NONE;
}

bool OB_Http_Client_get_headers(struct OB_Http_Client *client, struct OB_Http_Response *response) {
    assert(client != NULL);
    assert(response != NULL);

    struct curl_header *prev = NULL;
    struct curl_header *curr;
    size_t size = 0;
    while((curr = curl_easy_nextheader(client->curl, CURLH_HEADER, -1, prev)) != NULL) {
        size++;
        prev = curr;
    }
    prev = NULL;

    if(!OB_Http_Headers_reserve(&response->headers, size)) {
        return false;
    }

    if(!OB_Buffer_reserve(&response->headers.string_buffer, OB_HEADERS_BUFFER_SIZE)) {
        return false;
    }

    while((curr = curl_easy_nextheader(client->curl, CURLH_HEADER, -1, prev)) != NULL) {
        if(!OB_Http_Headers_append(&response->headers, curr->name, curr->value)) {
            return false;
        }
        prev = curr;
    }

    return true;
}

const char *OB_Http_Client_get_error(const struct OB_Http_Client *const client) {
    assert(client != NULL);

    return client->error;
}

void OB_Http_Request_init(struct OB_Http_Request *request) {
    assert(request != NULL);
    
    request->url                 = NULL;
    request->method              = OB_HTTP_METHOD_GET;
    request->follow_redirections = true;
    request->curl_headers        = NULL;
    
    OB_Body_init(&request->body);
    OB_Http_Headers_init(&request->headers);
}

void OB_Http_Request_free(struct OB_Http_Request *request) {
    assert(request != NULL);

    curl_slist_free_all(request->curl_headers);
    OB_Http_Headers_free(&request->headers);
    OB_Body_free(&request->body);
    OB_Http_Request_init(request);
}

bool OB_Http_Request_basic_auth(struct OB_Http_Request *const request, const char *const username, const char *const password) {
    assert(request != NULL);
    assert(username != NULL);
    assert(password != NULL);

    const int size = snprintf(NULL, 0, "Basic %s:%s", username, password);
    if(size < 0) {
        return false;
    }

    char *value = (char*)OB_MALLOC(((size_t)size + 1) * sizeof(char));
    if(value == NULL) {
        return false;
    }

    snprintf(value, (size_t)size + 1, "Basic %s:%s", username, password);

    const bool success = OB_Http_Headers_set(&request->headers, "authorization", value);

    OB_FREE(value);

    return success;
}

void OB_Http_Response_init(struct OB_Http_Response *response) {
    assert(response != NULL);

    OB_Body_init(&response->body);
    OB_Http_Headers_init(&response->headers);
    response->status_code = 0u;
}

void OB_Http_Response_free(struct OB_Http_Response *response) {
    assert(response != NULL);

    OB_Body_free(&response->body);
    OB_Http_Headers_free(&response->headers);
    OB_Http_Response_init(response);
}