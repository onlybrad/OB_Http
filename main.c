#include <stdio.h>
#include "src/http.h"

int main(void) {
    struct OB_Http_Client client;
    struct OB_Http_Request request;
    struct OB_Http_Response response;

    OB_Http_Request_init(&request);
    OB_Http_Response_init(&response);
    OB_Http_Client_init(&client); //can fail
    
    request.url = "https://pokeapi.co/api/v2/pokemon/pikachu";
    OB_Http_Headers_append(&request.headers, "Accept", "application/json"); //can fail
    OB_Http_Headers_append(&request.headers, "Cock", "Tail"); //can fail
    
    enum OB_Http_Error error;
    if((error = OB_Http_Client_fetch(&client, &request, &response)) == OB_HTTP_ERROR_NONE) {
    } else {
        fprintf(stderr, "Error: %s\n", client.error);
    }

    OB_Http_Request_free(&request);
    OB_Http_Response_free(&response); //response must be freed after a fetch to prevent memory leaks
    OB_Http_Client_free(&client);     //free the client once you finish using it to close the underlying curl handle

    return (int)error;
}