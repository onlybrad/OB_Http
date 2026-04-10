#include <assert.h>

#include "http.h"
#include "util.h"
#include "file.h"

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

static bool OB_Http_default_progress_callback(const size_t accumulated, size_t total, struct OB_ProgressData *const progress_data, const char *const message) {
    assert(accumulated > 0);
    assert(progress_data != NULL);
    assert(message != NULL);

    const int64_t time_elapsed = progress_data->current_time - progress_data->start_time;
    if(time_elapsed == 0) {
        return true;
    }

    const double speed = (double)accumulated / (double)time_elapsed;
    if(total == 0) {
        printf("???%% %s (%zuB) %.2lf MB/s.\n", message, accumulated, speed);
    } else {
        const double percentage = (double)accumulated * 100.0 / (double)total;
        printf("%.2f%% %s (%zuB) %.2lf MB/s.\n", percentage, message, accumulated, speed);
    }

    return true;
}

static bool OB_Http_default_download_progress_callback(const size_t accumulated, size_t total, struct OB_ProgressData *const progress_data) {
    assert(accumulated > 0);
    assert(progress_data != NULL);

    return OB_Http_default_progress_callback(accumulated, total, progress_data, "Downloaded");
}

static bool OB_Http_default_upload_progress_callback(const size_t accumulated, size_t total, struct OB_ProgressData *const progress_data) {
    assert(accumulated > 0);
    assert(progress_data != NULL);

    return OB_Http_default_progress_callback(accumulated, total, progress_data, "Uploaded");
}

static int OB_libcurl_progress_callback(void *user_data, const curl_off_t dl_total, const curl_off_t dl_now, const curl_off_t ul_total, const curl_off_t ul_now) {
    assert(user_data != NULL);

    struct OB_Progress *const progress = (struct OB_Progress*)user_data;
    struct OB_ProgressData *progress_data;
    size_t accumulated, total;

    if(ul_now != 0 && progress->upload.callback != NULL) {
        progress_data = &progress->upload;
        accumulated   = (size_t)ul_now;
        total         = (size_t)ul_total;
    } else if(dl_now != 0 && progress->download.callback != NULL) {
        progress_data = &progress->download;
        accumulated   = (size_t)dl_now;
        total         = (size_t)dl_total;
    } else {
        return 0;
    }

    if(progress_data->accumulated == accumulated) {
        return 0;
    }

    progress_data->current_time = OB_get_usec_timestamp();
    const bool success          = progress_data->callback(accumulated, total, progress_data);
    progress_data->accumulated  = accumulated;

    return !success;
}

static size_t OB_libcurl_file_callback(char *data, size_t size, size_t count, void *user_data) {
    assert(size == 1);
    assert(data != NULL);
    assert(user_data != NULL);

    return fwrite(data, size, count, (FILE*)user_data);
}

static size_t OB_libcurl_buffer_callback(char *data, size_t size, size_t count, void *user_data) {
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

static enum OB_Http_Error OB_Http_Client_prepare_request_headers(struct OB_Http_Client *const client, struct OB_Http_Request *const request) {
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

static enum OB_Http_Error OB_Http_Client_decode_body(struct OB_Http_Client *const client, struct OB_Http_Response *const response) {
    assert(client != NULL);
    assert(response != NULL);
    assert(response->body_source.type == OB_BODY_SOURCE_TYPE_BUFFER);

    struct curl_header *content_type;
    if(curl_easy_header(client->curl, "content-type", 0, CURLH_HEADER, -1, &content_type) != CURLHE_OK) {
        return OB_HTTP_ERROR_CURL;
    }

    struct OB_Body *const body = &response->body_source.value.body; 

    static const char text_plain[] = "text/plain";
    if(strncmp(content_type->value, text_plain, sizeof(text_plain) - 1) == 0) {
        response->body_source.value.body.type = OB_BODY_TYPE_BUFFER;
        return OB_HTTP_ERROR_NONE;
    }

    static const char application_json[] = "application/json";
    if(strncmp(content_type->value, application_json, sizeof(application_json) - 1) == 0) {
        struct OB_Buffer buffer;
        const bool success = OB_Body_move_buffer(body, &buffer);
        assert(success);
        (void)success;

        switch(OB_Body_parse_json(body, &buffer)) {
        case OB_JSON_ERROR_NONE:
            OB_Buffer_free(&buffer);
            return OB_HTTP_ERROR_NONE;
        
        case OB_JSON_ERROR_TOO_LARGE:
            OB_Body_set_buffer(body, &buffer);
            return OB_HTTP_ERROR_TOO_LARGE;
        
        case OB_JSON_ERROR_PARSING:
            OB_Body_set_buffer(body, &buffer);
            return OB_HTTP_ERROR_JSON_PARSING;
        }
    }

    static const char text_html[] = "text/html";
    if(strncmp(content_type->value, text_html, sizeof(text_html) - 1) == 0) {
        body->type = OB_BODY_TYPE_HTML;
        return OB_HTTP_ERROR_NONE;
    }

    static const char application_xml[] = "application/xml";
    if(strncmp(content_type->value, application_xml, sizeof(application_xml) - 1) == 0) {
        body->type = OB_BODY_TYPE_XML;
        return OB_HTTP_ERROR_NONE;
    }

    static const char urlencoded[] = "application/x-www-form-urlencoded";
    if(strncmp(content_type->value, urlencoded, sizeof(urlencoded) - 1) == 0) {
        body->type = OB_BODY_TYPE_URL_ENCODED;
        return OB_HTTP_ERROR_NONE;
    }

    static const char multipart_form_data[] = "multipart/form-data";
    if(strncmp(content_type->value, multipart_form_data, sizeof(multipart_form_data) - 1) == 0) {
        body->type = OB_BODY_TYPE_FORM_DATA;
        return OB_HTTP_ERROR_NONE;
    }
    
    static const char octect_stream[] = "application/octet-stream";
    if(strncmp(content_type->value, octect_stream, sizeof(octect_stream) - 1) == 0) {
        body->type = OB_BODY_TYPE_BUFFER;
        return OB_HTTP_ERROR_NONE;
    }

    body->type = OB_BODY_TYPE_BUFFER;
    return OB_HTTP_ERROR_NONE;
}

static bool OB_BodySource_free(struct OB_BodySource *const body_source) {
    assert(body_source != NULL);

    if(body_source->type == OB_BODY_SOURCE_TYPE_BUFFER) {
        OB_Body_free(&body_source->value.body);
    } else if(body_source->value.file != NULL) {
        if(fclose(body_source->value.file) == EOF) {
            return false;
        }
    }

    return true;
}

static bool OB_BodySource_set_file_path(struct OB_BodySource *const body_source, const char *const file_path, bool is_readmode) {
    assert(body_source != NULL);
    assert(file_path != NULL);

    if(!OB_BodySource_free(body_source)) {
        return false;
    }
    body_source->type = OB_BODY_SOURCE_TYPE_FILE;
    return (body_source->value.file = OB_fopen(file_path, is_readmode)) != NULL;
}

static bool OB_BodySource_set_file(struct OB_BodySource *const body_source, FILE *const file) {
    assert(body_source != NULL);
    assert(file != NULL);

    if(!OB_BodySource_free(body_source)) {
        return false;
    }
    body_source->type = OB_BODY_SOURCE_TYPE_FILE;
    body_source->value.file = file;
    return true;
}

static struct OB_Body *OB_BodySource_get_body(struct OB_BodySource *const body_source) {
    assert(body_source != NULL);

    return body_source->type == OB_BODY_SOURCE_TYPE_BUFFER
        ? &body_source->value.body
        : NULL;
}

static void OB_ProgressData_init(struct OB_ProgressData *const progress_data) {
    assert(progress_data != NULL);

    progress_data->callback     = NULL;
    progress_data->user_data    = NULL;
    progress_data->accumulated  = 0;
    progress_data->start_time   = 0;
    progress_data->current_time = 0;
}

bool OB_Http_Client_init(struct OB_Http_Client *const client) {
    assert(client != NULL);

    client->curl                         = NULL;
    client->max_redirections             = OB_MAX_REDIRECTIONS;
    client->get_headers                  = false;
    client->get_body                     = true;
    OB_ProgressData_init(&client->progress.download);
    OB_ProgressData_init(&client->progress.upload);
    memset(client->error, 0, sizeof(client->error));
    
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

enum OB_Http_Error OB_Http_Client_fetch(struct OB_Http_Client *const client, struct OB_Http_Request *const request, struct OB_Http_Response *const response) {
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
        OB_CURL_SETOPT(client->curl, CURLOPT_XFERINFOFUNCTION, OB_libcurl_progress_callback);
        OB_CURL_SETOPT(client->curl, CURLOPT_XFERINFODATA, &client->progress);

        if(client->progress.download.callback != NULL || client->progress.upload.callback != NULL) {
            OB_CURL_SETOPT(client->curl, CURLOPT_NOPROGRESS, 0L);
        } else {
            OB_CURL_SETOPT(client->curl, CURLOPT_NOPROGRESS, 1L);
        }

        if(response->body_source.type == OB_BODY_SOURCE_TYPE_BUFFER) {
            OB_CURL_SETOPT(client->curl, CURLOPT_WRITEDATA, &response->body_source.value.body.u.buffer);
            OB_CURL_SETOPT(client->curl, CURLOPT_WRITEFUNCTION, OB_libcurl_buffer_callback);
            OB_CURL_SETOPT(client->curl, CURLOPT_FAILONERROR, 0L);

            OB_Body_use_buffer(&response->body_source.value.body);
            if(!OB_Buffer_reserve(&response->body_source.value.body.u.buffer, CURL_MAX_WRITE_SIZE)) {
                return OB_HTTP_ERROR_MALLOC;
            }
        } else {
            OB_CURL_SETOPT(client->curl, CURLOPT_WRITEDATA, response->body_source.value.file);
            OB_CURL_SETOPT(client->curl, CURLOPT_WRITEFUNCTION, OB_libcurl_file_callback);
            OB_CURL_SETOPT(client->curl, CURLOPT_FAILONERROR, 1L);
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

    client->progress.upload.start_time   = 
    client->progress.download.start_time = OB_get_usec_timestamp();
    OB_CURL_PERFORM(client->curl);

    long status_code;
    OB_CURL_GETINFO(client->curl, CURLINFO_RESPONSE_CODE, &status_code);
    response->status_code = (unsigned)status_code;

    if(client->get_headers && !OB_Http_Client_get_headers(client, response)) {
        return OB_HTTP_ERROR_MALLOC;   
    }

    if(client->get_body) {
        if(response->body_source.type == OB_BODY_SOURCE_TYPE_BUFFER) {
            return OB_Http_Client_decode_body(client, response);
        }
        if(response->body_source.value.file != NULL) {
            if(fflush(response->body_source.value.file) == EOF) {
                return OB_HTTP_ERROR_FFLUSH;
            }
        }
    }
    
    return OB_HTTP_ERROR_NONE;
}

bool OB_Http_Client_get_headers(struct OB_Http_Client *const client, struct OB_Http_Response *const response) {
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

void OB_Http_Client_on_upload(struct OB_Http_Client *const client, OB_ProgressCallback callback, void *const user_data) {
    assert(client != NULL);

    client->progress.upload.callback  = callback;
    client->progress.upload.user_data = user_data;
}

void OB_Http_Client_on_download(struct OB_Http_Client *const client, OB_ProgressCallback callback, void *const user_data) {
    assert(client != NULL);

    client->progress.download.callback  = callback;
    client->progress.download.user_data = user_data;
}

void OB_Http_Client_show_upload_progress(struct OB_Http_Client *const client) {
    assert(client != NULL);

    OB_Http_Client_on_upload(client, OB_Http_default_upload_progress_callback, NULL);
}

void OB_Http_Client_show_download_progress(struct OB_Http_Client *const client) {
    assert(client != NULL);

    OB_Http_Client_on_download(client, OB_Http_default_download_progress_callback, NULL);
}

void OB_Http_Request_init(struct OB_Http_Request *const request) {
    assert(request != NULL);
    
    request->url                 = NULL;
    request->method              = OB_HTTP_METHOD_GET;
    request->follow_redirections = true;
    request->curl_headers        = NULL;
    
    OB_Body_init(&request->body_source.value.body);
    request->body_source.type = OB_BODY_SOURCE_TYPE_BUFFER;
    
    OB_Http_Headers_init(&request->headers);
}

void OB_Http_Request_free(struct OB_Http_Request *const request) {
    assert(request != NULL);

    curl_slist_free_all(request->curl_headers);
    OB_Http_Headers_free(&request->headers);
    if(request->body_source.type == OB_BODY_SOURCE_TYPE_BUFFER) {
        OB_Body_free(&request->body_source.value.body);
    } else {
        fclose(request->body_source.value.file);
    }
    OB_Http_Request_init(request);
}

void OB_Http_Request_set_url(struct OB_Http_Request *const request, const char *const url) {
    assert(request != NULL);
    assert(url != NULL);

    request->url = url;
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

bool OB_Http_Request_set_file(struct OB_Http_Request *const request, FILE *const file) {
    assert(request != NULL);
    assert(file != NULL);

    return OB_BodySource_set_file(&request->body_source, file);
}

bool OB_Http_Request_set_file_path(struct OB_Http_Request *const request, const char *path) {
    assert(request != NULL);
    assert(path != NULL);

    return OB_BodySource_set_file_path(&request->body_source, path, true);
}

void OB_Http_Response_init(struct OB_Http_Response *const response) {
    assert(response != NULL);

    OB_Body_init(&response->body_source.value.body);
    response->body_source.type  = OB_BODY_SOURCE_TYPE_BUFFER;

    OB_Http_Headers_init(&response->headers);
    response->status_code          = 0u;
}

void OB_Http_Response_free(struct OB_Http_Response *const response) {
    assert(response != NULL);

    OB_BodySource_free(&response->body_source);
    OB_Http_Headers_free(&response->headers);
    OB_Http_Response_init(response);
}

bool OB_Http_Response_set_file(struct OB_Http_Response *const response, FILE *const file) {
    assert(response != NULL);
    assert(file != NULL);

    return OB_BodySource_set_file(&response->body_source, file);
}

bool OB_Http_Response_set_file_path(struct OB_Http_Response *const response, const char *path) {
    assert(response != NULL);
    assert(path != NULL);

    return OB_BodySource_set_file_path(&response->body_source, path, false);
}

unsigned OB_Http_Response_get_status_code(const struct OB_Http_Response *const response) {
    assert(response != NULL);

    return response->status_code;
}

struct OB_Body *OB_Http_Response_get_body(struct OB_Http_Response *const response) {
    assert(response != NULL);
    
    return OB_BodySource_get_body(&response->body_source);
}