#include <stdio.h>
#include "src/http.h"

int main(void) {
    struct OB_HttpClient client;
    struct OB_HttpRequest request;
    struct OB_HttpResponse response;

    if(!OB_HttpClient_init(&client)) {
        return -1;
    }
    client.max_redirections = 20;            //default value

    OB_HttpRequest_init(&request);
    request.url = "https://www.google.com";
    request.method = OB_HTTP_METHOD_GET;     //default value
    request.follow_redirections = true;      //default value

    OB_HttpResponse_init(&response);
    
    enum OB_HttpError error;
    if((error = OB_HttpClient_fetch(&client, &request, &response)) == OB_HTTP_ERROR_NONE) {
        printf("Status Code: %u\n", response.status_code);

        for(size_t i = 0; i < response.header_count; i++) {
            printf("%s : %s\n", response.headers[i].name, response.headers[i].value);
        }
    } else {
        fprintf(stderr, "Error: %s\n", client.error);
    }

    OB_HttpResponse_free(&response); //response must be freed after a fetch to prevent memory leaks
    OB_HttpClient_free(&client);     //free the client once you finish using it to close the underlying curl handle

    return (int)error;
}