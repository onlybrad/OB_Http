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
    OB_Http_Headers_append(&request.headers, "accept", "application/json"); //can fail
    
    bool success;
    const char *value;
    struct CJSON_QueryBuilder query_builder;

    enum OB_Http_Error error;
    if((error = OB_Http_Client_fetch(&client, &request, &response)) != OB_HTTP_ERROR_NONE) {
        fprintf(stderr, "Error: %s\n", client.error);
        goto end;
    }

    printf("Status Code: %u\n", response.status_code);
    if(response.status_code != 200) {
        goto end;
    }

    if(response.body.type != OB_HTTP_BODY_TYPE_JSON) {
        fputs("Body is not json.\n", stderr);
        goto end;
    }

    query_builder = CJSON_get_query_builder(&response.body.u.json_parser.json);
    CJSON_QueryBuilder_format(&query_builder, "kkkkk", "sprites", "versions", "generation-i", "yellow", "front_default");

    value = CJSON_as_string(query_builder.json, &success);
    if(!success) {
        fputs("Failed JSON query or value is not a string.\n", stderr);
        goto end;
    }

    puts(value);

end:
    OB_Http_Request_free(&request);
    OB_Http_Response_free(&response);
    OB_Http_Client_free(&client);
    return (int)error;
}