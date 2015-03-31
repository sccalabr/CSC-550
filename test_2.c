#include "squirrelDBClient.h"
#include <stdlib.h>
#include <string.h>

#define ADDR "129.65.156.26"
//#define ADDR "127.0.0.1"
/*
   Slave Node Crash Recovery - Same things are printed as above. Also printed is
   a message when the master node realizes that one of the slave nodes has gone 
   down. The following tests will be run:
	Tests -
            put(‘key_1’, ‘value’)
            put(‘key_2’, ‘value’)
            shutdown node 1
            get(‘key_1’)
            get('key_2')

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

    printf("---------- STARTING TEST_2 ----------\n");
    fflush(stdout);

    char *key_1 = (char *) calloc(sizeof (char), 16);
    sprintf(key_1, "key_1");
    char *key_2 = (char *) calloc(sizeof (char), 16);
    sprintf(key_2, "key_2");
    char *value = (char *)calloc(sizeof(char), sizeof("value"));
    sprintf(value, "value");
    char *resp_val = (char *)calloc(sizeof(char), sizeof("value"));
    
    printf("Running put('%s', '%s') ... ", key_1, value);
    response = squirrelDB_put((uint8_t *) key_1, (uint8_t*) value, 
            sizeof("value"));
    check_response(response);
    printf("    It should have succeeded.\n");
    
    printf("\nRunning put('%s', '%s') ... ", key_2, value);
    response = squirrelDB_put((uint8_t *) key_2, (uint8_t*) value, 
            sizeof("value"));
    check_response(response);
    printf("    It should have succeeded.\n");
    
    printf("\nShutting down node 1 ... ");
    response = squirrelDB_slave_go_down(1);
    check_response(response);
    printf("    It should have succeeded.\n");
    
    printf("\nRunning get('%s') ... ", key_1);
    response = squirrelDB_get((uint8_t *) key_1, (uint8_t*) resp_val);
    check_response(response);
    printf("    It should have succeeded.\n");
    printf("    Value expected: '%s'\n", value);
    printf("    Value returned: '%s'\n", resp_val);
    
    printf("\nRunning get('%s') ... ", key_2);
    response = squirrelDB_get((uint8_t *) key_2, (uint8_t*) resp_val);
    check_response(response);
    printf("    It should have succeeded.\n");
    printf("    Value expected: '%s'\n", value);
    printf("    Value returned: '%s'\n", resp_val);
    
    free(key_1);
    free(key_2);
    free(value);
    free(resp_val);
    
    printf("---------- TEST_2 OVER ----------\n");
    fflush(stdout);

    squirrelDB_finalize();

    return 1;
}
