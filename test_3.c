#include "squirrelDBClient.h"
#include <stdlib.h>
#include <string.h>

#define ADDR_1 "129.65.156.26"
#define ADDR_2 "129.65.156.27"
//#define ADDR_1 "127.0.0.1"
//#define ADDR_2 "127.0.0.1"

/*
    Master Node Crash Recovery - Beforehand we will run the database and call 
        put(‘key_1’, ‘value’) through put(‘key_50’ ‘value’) and run the 
        database long enough for the logs and master data tables to be flushed 
        to all slave nodes. We will start up the database and restore to that 
        save position. Then we run commands (shown below). We will then stop 
        the master nodes termination before that data can be flushed to the 
        slave nodes. After master termination, one of the slave nodes will be 
        chosen as new master and restore to last saved point. We will show that 
        data between flushes has been lost, but all previous data is still 
        there by doing a get(‘key_1’) through get(‘key_52’).
            put(‘key_51’, ‘value’)
            put(‘key_52’, ‘value’)
            update(‘key_34’, ‘new value’)
            update(‘key_11’, ‘this is the new value for key_11’)
            delete(‘key_16’)
            get(‘key_1’) through
            get('key_52)
            


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
        response = squirrelDB_init(ADDR_1);
    } while(response == FAILURE);
        
    printf("Connected to DRUMSDB\n");
    fflush(stdout);

    printf("---------- STARTING TEST_3 ----------\n");
    fflush(stdout);
    
    char *key = (char *) calloc(sizeof (char), 16);
    char *value = (char *)calloc(sizeof(char), sizeof("value"));
    sprintf(value, "value");
    
    int i;
    
    //THIS FOR LOOP WAS TO POPULATE FOR TEST_3
    /*
    for(i = 1; i <= 50; i++) {
	sprintf(key, "key_%d", i);
	squirrelDB_put((uint8_t *) key, (uint8_t *) value, sizeof("value"));
    }
    */
        
    //set to size of largest string to be returned
    char *resp_val = (char *)calloc(sizeof(char), 
            sizeof("this is the new value for key_11"));
    
    sprintf(key, "key_51");
    printf("Running put('%s', '%s') ... ", key, value);
    response = squirrelDB_put((uint8_t *) key, (uint8_t *) value, 
            sizeof("value"));
    check_response(response);
    printf("    It should have succeeded.\n");
    
    sprintf(key, "key_52");
    printf("\nRunning put('%s', '%s') ... ", key, value);
    response = squirrelDB_put((uint8_t *) key, (uint8_t *) value, 
            sizeof("value"));
    check_response(response);
    printf("    It should have succeeded.\n");
    
    sprintf(key, "key_34");
    value = (char *)realloc(value, sizeof(char) * sizeof("new value"));
    sprintf(value, "new value");
    printf("\nRunning update('%s', '%s') ... ", key, value);
    response = squirrelDB_update((uint8_t *)key, (uint8_t *)value, 
            sizeof("new value"));
    check_response(response);
    printf("    It should have succeeded.\n");
    
    sprintf(key, "key_11");
    value = (char *)realloc(value, sizeof(char) * 
            sizeof("this is the new value for key_11"));
    sprintf(value, "this is the new value for key_11");
    printf("\nRunning update('%s', '%s') ... ", key, value);
    response = squirrelDB_update((uint8_t *)key, (uint8_t *)value, 
            sizeof("this is the new value for key_11"));
    check_response(response);
    printf("    It should have succeeded.\n");
    
    sprintf(key, "key_16");
    printf("\nRunning delete('%s') ... ", key);
    response = squirrelDB_delete(key);
    check_response(response);
    printf("    It should have succeeded.\n");
    
    
    for(i = 1; i <= 52; i++) {
        sprintf(key, "key_%d", i);
        memset(resp_val, 0, sizeof("this is the new value for key_11"));
        printf("\nRunning get('%s') ... ", key);
        response = squirrelDB_get((uint8_t *) key, (uint8_t*) resp_val);
        check_response(response);
        
        if(i != 16) {
            printf("    It should have succeeded.\n");
            printf("    Value returned: '%s'\n", resp_val);
        } else {
           printf("    It should have failed.\n"); 
        }
    }

    printf("\nShutting down master ... ");
    response = squirrelDB_master_go_down();
    check_response(response);
    printf("    It should have succeeded.\n");
    
    printf("\nCONNECTION TO DATABASE LOST. TRYING TO RESORE CONNECTION.\n");
    fflush(stdout);
    
    do {
        sleep(20);
        printf("\nTrying ip addr %s ... ", ADDR_2);
        response = squirrelDB_init(ADDR_2);
        check_response(response);
    } while(response == FAILURE);
    
    for(i = 1; i <= 50; i++) {
        sprintf(key, "key_%d", i);
        printf("\nRunning get('%s') ... ", key);
        response = squirrelDB_get((uint8_t *) key, (uint8_t*) resp_val);
        check_response(response);
        printf("    It should have succeeded.\n");
        printf("    Value returned: '%s'\n", resp_val);
    }
    
    sprintf(key, "key_%d", 51);
    printf("\nRunning get('%s') ... ", key);
    response = squirrelDB_get((uint8_t *) key, (uint8_t*) resp_val);
    check_response(response);
    printf("    It should have failed.\n");
        
    sprintf(key, "key_%d", 52);
    printf("\nRunning get('%s') ... ", key);
    response = squirrelDB_get((uint8_t *) key, (uint8_t*) resp_val);
    check_response(response);
    printf("    It should have failed.\n");
    
    free(key);
    free(resp_val);
    free(value);
    
    printf("---------- TEST_3 OVER ----------\n");
    fflush(stdout);

    squirrelDB_finalize();

    return 1;
}
