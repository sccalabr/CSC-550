#include "squirrelDBClient.h"
#include <stdlib.h>
#include <string.h>

#define ADDR "129.65.156.26"
//#define ADDR "127.0.0.1"

/*
    Put test - put(‘key’, ‘value’)
    Get test - get(‘key’)
    Update to larger value test - update(‘key’, ‘new much much longer value’)
    Get updated data test - get(‘key’)
    Update to smaller value test - update(‘key’, ‘smaller value’)
    Get updated data test - get(‘key’)
    Delete test - delete(‘key’)
    Put existing key test - put(‘key’, ‘value’), then put(‘key’, ‘new value’)
    Update non existent key - update(‘unknown key’, ‘value’)
    Delete non existent key - delete(‘unknown key’,  ‘value’)
    Clearing Database Step - Delete(‘key’, ‘value’)
    Get non existent data - get(‘key’)
    key too long test - put(‘This key is really really far too long’, ‘test’) 
    Too much data test - put(‘long data’, ‘a’ * 500000001)
 */

void check_response(int response) {
    if(response == SUCCESS) {
        printf("succeeded.\n");
        fflush(stdout);
    } else {
        printf("failed.\n");
        fflush(stdout);
    }
}

int main(int argc, char **argv) {
    printf("---------- CONNECTING TO DATABASE ----------\n");
    int response;
    do {
        response = squirrelDB_init(ADDR);
    } while(response == FAILURE);
        
    printf("Connected to SquirrelDB\n");
    fflush(stdout);

    printf("---------- STARTING TEST_1 ----------\n");
    fflush(stdout);

    char *key = (char *) calloc(sizeof (char), 16);
    sprintf(key, "key");
    char *value = (char *)calloc(sizeof(char), sizeof("value"));
    sprintf(value, "value");
    char *resp_val = (char *)calloc(sizeof(char), sizeof("value"));
    
    printf("Running put('%s', '%s') ... ", key, value);
    response = squirrelDB_put((uint8_t *) key, (uint8_t*) value, 
            sizeof("value"));
    check_response(response);
    printf("    It should have succeeded.\n");
    
    printf("\nRunning get('%s') ... ", key);
    response = squirrelDB_get((uint8_t *) key, (uint8_t*) resp_val);
    check_response(response);
    printf("    It should have succeeded.\n");
    printf("    Value expected: '%s'\n", value);
    printf("    Value returned: '%s'\n", resp_val);
    
    value = (char *)realloc(value, sizeof("new much much longer value"));
    sprintf(value, "new much much longer value");
    printf("\nRunning update('%s', '%s') ... ", key, value);
    response = squirrelDB_update((uint8_t *)key, (uint8_t *)value, 
            sizeof("new much much longer value"));
    check_response(response);
    printf("    It should have succeeded.\n");
    
    printf("\nRunning get('%s') ... ", key);
    resp_val = (char *)realloc(resp_val, sizeof("new much much longer value"));
    response = squirrelDB_get((uint8_t *) key, (uint8_t*) resp_val);
    check_response(response);
    printf("    It should have succeeded.\n");
    printf("    Value expected: '%s'\n", value);
    printf("    Value returned: '%s'\n", resp_val);

    value = (char *)realloc(value, sizeof("smaller value"));
    sprintf(value, "smaller value");
    printf("\nRunning update('%s', '%s') ... ", key, value);
    response = squirrelDB_update((uint8_t *)key, (uint8_t *)value, 
            sizeof("smaller value"));
    check_response(response);
    printf("    It should have succeeded.\n");
    
    printf("\nRunning get('%s') ... ", key);
    resp_val = (char *)realloc(resp_val, sizeof("smaller value"));
    response = squirrelDB_get((uint8_t *) key, (uint8_t*) resp_val);
    check_response(response);
    printf("    It should have succeeded.\n");
    printf("    Value expected: '%s'\n", value);
    printf("    Value returned: '%s'\n", resp_val);
    
    printf("\nRunning delete('%s') ... ", key);
    response = squirrelDB_delete(key);
    check_response(response);
    printf("    It should have succeeded.\n");
    
    value = (char *)realloc(value, sizeof("value"));
    sprintf(value, "value");
    printf("\nRunning put('%s', '%s') ... ", key, value);
    response = squirrelDB_put((uint8_t *) key, (uint8_t*) value, 
            sizeof("value"));
    check_response(response);
    printf("    It should have succeeded.\n");
    
    value = (char *)realloc(value, sizeof("new value"));
    sprintf(value, "new value");
    printf("\nRunning put('%s', '%s') ... ", key, value);
    response = squirrelDB_put((uint8_t *) key, (uint8_t*) value, 
            sizeof("new value"));
    check_response(response);
    printf("    It should have failed.\n");
    
    sprintf(key, "unknown key");
    value = (char *)realloc(value, sizeof("value"));
    sprintf(value, "value");
    printf("\nRunning update('%s', '%s') ... ", key, value);
    response = squirrelDB_update((uint8_t *) key, (uint8_t*) value, 
            sizeof("value"));
    check_response(response);
    printf("    It should have failed.\n");
    
    printf("\nRunning delete('%s') ... ", key);
    response = squirrelDB_delete((uint8_t *) key);
    check_response(response);
    printf("    It should have failed.\n");
    
    key = (char *)memset(key, 0, 16);
    sprintf(key, "key");
    printf("\nRunning delete('%s') ... ", key);
    response = squirrelDB_delete((uint8_t *) key);
    check_response(response);
    printf("    It should have succeeded.\n");
    
    resp_val = (char *)realloc(resp_val, sizeof("value"));
    printf("\nRunning get('%s') ... ", key);
    response = squirrelDB_get((uint8_t *) key, (uint8_t *)resp_val);
    check_response(response);
    printf("    It should have failed.\n");
    
    char *too_long_key = (char *)calloc(sizeof(char), 
            sizeof("This key is really really far too long"));
    sprintf(too_long_key, "This key is really really far too long");
    printf("\nRunning put('%s', '%s') ... ", too_long_key, value);
    response = squirrelDB_put((uint8_t *) too_long_key, (uint8_t*) value, 
            sizeof("value"));
    check_response(response);
    printf("    It should have failed.\n");
    
    //This test fakes sending too large a value
    printf("\nRunning put('%s', 'a' * 500000001) ... ", key);
    response = squirrelDB_put((uint8_t *) key, (uint8_t*) NULL, 500000001);
    check_response(response);
    printf("    It should have failed.\n");
    
    free(key);
    free(value);
    free(resp_val);
    free(too_long_key);
    
    printf("---------- TEST_1 OVER ----------\n");
    fflush(stdout);

    squirrelDB_finalize();

    return 1;
}
