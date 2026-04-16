#include <stdio.h>
#include <stdlib.h>
#include "../src/http.h"

int main(void) {
    OB_Http_init(); //can fail

    struct OB_Http_Client client;
    struct OB_Http_Request request;
    struct OB_Http_Response response;

    OB_Http_Request_init(&request);
    OB_Http_Response_init(&response);
    OB_Http_Client_init(&client); //can fail

    OB_Http_Client_show_download_progress(&client);
    
    OB_Http_Request_set_url(&request, "https://pokeapi.co/api/v2/pokemon/pikachu");
    OB_Http_Headers_append(&request.headers, "accept", "application/json"); //can fail
    
    int exit_code = EXIT_FAILURE;
    do {
        if(OB_Http_Client_fetch(&client, &request, &response) != OB_HTTP_ERROR_NONE) {
            fprintf(stderr, "Error: %s\n", OB_Http_Client_get_error(&client));
            break;
        }

        const unsigned status_code = OB_Http_Response_get_status_code(&response);
        printf("Status Code: %u\n", status_code);
        if(status_code != 200) {
            break;
        }

        struct OB_Http_Body *const body = OB_Http_Response_get_body(&response);
        struct CJSON *const json = OB_Http_Body_get_json(body);
        if(json == NULL) {
            fputs("Body is not json.\n", stderr);
            break;
        }

        struct CJSON_QueryBuilder query_builder = CJSON_get_query_builder(json);
        CJSON_QueryBuilder_format(&query_builder, "kkkkk", 
            "sprites", "versions", "generation-i", "yellow", "front_default"
        );

        bool success;
        const char *const value = CJSON_as_string(query_builder.json, &success);
        if(!success) {
            fputs("Failed JSON query or value is not a string.\n", stderr);
            break;
        }

        puts(value);

        exit_code = EXIT_SUCCESS;
    } while(0);

    OB_Http_Request_free(&request);
    OB_Http_Response_free(&response);
    if(exit_code == EXIT_FAILURE) {
        OB_Http_Client_free(&client);
        return EXIT_FAILURE;
    }
    
    OB_Http_Request_set_url(&request, "https://pokeapi.co/api/v2/pokemon?some_useless_parameter=123#some_random_hash_here");
    OB_Http_Request_set_query_param(&request, "limit", "100000"); //can fail
    OB_Http_Request_set_query_param(&request, "offset", "0"); //can fail
    OB_Http_Headers_append(&request.headers, "accept", "application/json"); //can fail
    
    exit_code = EXIT_FAILURE;
    do {
        if(OB_Http_Client_fetch(&client, &request, &response) != OB_HTTP_ERROR_NONE) {
            fprintf(stderr, "Error: %s\n", OB_Http_Client_get_error(&client));
            break;
        }

        const unsigned status_code = OB_Http_Response_get_status_code(&response);
        printf("Status Code: %u\n", status_code);
        if(status_code != 200) {
            break;
        }

        struct OB_Http_Body *const body = OB_Http_Response_get_body(&response);
        struct CJSON *const json = OB_Http_Body_get_json(body);
        if(json == NULL) {
            fputs("Body is not json.\n", stderr);
            break;
        }

        struct CJSON_QueryBuilder query_builder = CJSON_get_query_builder(json);
        CJSON_QueryBuilder_format(&query_builder, "ki", "results", 0);

        bool success;
        const struct CJSON_Object *pokemon = CJSON_as_object(query_builder.json, &success);
        if(!success) {
            fputs("Failed JSON query or value is not an object.\n", stderr);
            break;
        }

        const char *const name = CJSON_Object_get_string(pokemon, "name", &success);
        if(!success) {
            fputs("Failed to extract name from object, either the name key doesn't exist or the value is not a string.\n", stderr);
            break; 
        }

        const char *const url = CJSON_Object_get_string(pokemon, "url", &success);
        if(!success) {
            fputs("Failed to extract url from object, either the url key doesn't exist or the value is not a string.\n", stderr);
            break;
        }

        printf("%s %s\n", name, url);

        exit_code = EXIT_SUCCESS;
    } while(0);

    OB_Http_Request_free(&request);
    OB_Http_Response_free(&response);
    OB_Http_Client_free(&client);
    OB_Http_free();

    return exit_code;
}