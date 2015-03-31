#include "perf_eval.h"

#define ADDR "129.65.156.26"

void rand_string(char *str, size_t size)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if (size) {
	size_t n;
        for (n = 0; n < size - 1; n++) {
            int key = rand() % (int) (sizeof charset - 1);
            str[n] = charset[key];
        }
        
        str[n]= '\0';
    }
}

int main(int argc, char **argv) {
    srand(time(NULL));
    
    printf("---------- CONNECTING TO DATABASE ----------\n");
    int response;
    do {
        sleep(5);
        response = squirrelDB_init(ADDR);
    } while(response == FAILURE);
        
    printf("Connected to SquirrelDB\n");
    fflush(stdout);
    
    printf("---------- POPULATING DATABASE ----------\n");
    fflush(stdout);
  
    char *key = (char *) calloc(sizeof (char), 16);
    
    int *num_value = (int *)calloc(sizeof(int), 1);
    char *med_len_value = (char *)calloc(sizeof(char), 50);
    char *long_len_value = (char *)calloc(sizeof(char), 500);
    
    int avail_mem = (MEM_SIZE / MEM_BLOCK_SIZE) * (MEM_BLOCK_SIZE - 4);
    
    int i;
    if(DATA_TYPE == 1) {
        for(i = 0; i < 99; i++) {
            sprintf(key, "key_%d", i);
            *num_value = rand();
            printf("put %d\n", *num_value);
            fflush(stdout);
            response = squirrelDB_put((uint8_t *)key, (uint8_t *)num_value, 
                    sizeof(int));
        }
    } else if(DATA_TYPE == 2) {
        for(i = 0; i < 99; i++) {
            sprintf(key, "key_%d", i);
            rand_string(med_len_value, 50);
            printf("put %s\n", med_len_value);
            fflush(stdout);
            response = squirrelDB_put((uint8_t *)key, 
                    (uint8_t *)med_len_value, 50);
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
        }
    } else if(DATA_TYPE == 3) {
        for(i = 0; i < 99; i++) {
            sprintf(key, "key_%d", i);
            rand_string(long_len_value, 500);
            printf("put %s\n", long_len_value);
            fflush(stdout);
            response = squirrelDB_put((uint8_t *)key, 
                    (uint8_t *)long_len_value, 500);
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
        }
    } else if(DATA_TYPE == 4) {
        for(i = 0; i < 50; i++) {
            sprintf(key, "key_%d", i);
            *num_value = rand();
            printf("put %d\n", *num_value);
            fflush(stdout);
            response = squirrelDB_put((uint8_t *)key, 
                    (uint8_t *)num_value, sizeof(int));
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
        }
        
        for(i = 50; i < 85; i++) {
            sprintf(key, "key_%d", i);
            rand_string(med_len_value, 50);
            printf("put %s\n", med_len_value);
            fflush(stdout);
            response = squirrelDB_put((uint8_t *)key, 
                    (uint8_t *)med_len_value, 50);
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
        }
        
        for(i = 85; i < 99; i++) {
            sprintf(key, "key_%d", i);
            rand_string(long_len_value, 500);
            printf("put %s\n", long_len_value);
            fflush(stdout);
            response = squirrelDB_put((uint8_t *)key, 
                    (uint8_t *)long_len_value, 500);
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
        }
    }
    
    printf("Database populated\n");
    fflush(stdout);

    printf("---------- STARTING PERF. EVAL. ----------\n");
    fflush(stdout);
    
    start_collecting();
    
    uint8_t *resp_value;
    
    if(DATA_TYPE == 1) {
        for(i = 100; i < 200; i++) {
            sprintf(key, "key_%d", i);
            
            *num_value = rand();
            printf("put %d\n", *num_value);
            fflush(stdout);
            response = squirrelDB_put((uint8_t *) key, 
                    (uint8_t *) num_value, sizeof(int));
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
            
            resp_value = (uint8_t *)calloc(sizeof(int), 1);
            response = squirrelDB_get((uint8_t *) key, (uint8_t *) resp_value);
            free(resp_value);
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
            
            *num_value = rand();
            printf("update %d\n", *num_value);
            fflush(stdout);
            response = squirrelDB_update((uint8_t *)key, 
                    (uint8_t *)num_value, sizeof(int));
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
            
            response = squirrelDB_delete(key);
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
        }
    } else if(DATA_TYPE == 2) {
        for(i = 100; i < 200; i++) {
            sprintf(key, "key_%d", i);
            
            rand_string(med_len_value, 50);
            printf("put %s\n", med_len_value);
            fflush(stdout);
            response = squirrelDB_put((uint8_t *) key, 
                    (uint8_t *) med_len_value, 50);
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
            
            resp_value = (uint8_t *)calloc(sizeof(char), 50);
            response = squirrelDB_get((uint8_t *) key, (uint8_t *) resp_value);
            free(resp_value);
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
            
            rand_string(med_len_value, 50);
            printf("update %s\n", med_len_value);
            fflush(stdout);
            response = squirrelDB_update((uint8_t *)key, 
                    (uint8_t *)med_len_value, 50);
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
            
            response = squirrelDB_delete(key);
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
        }
    } else if(DATA_TYPE == 3) {
        for(i = 100; i < 200; i++) {
            sprintf(key, "key_%d", i);
            
            rand_string(long_len_value, 500);
            printf("put %s\n", long_len_value);
            fflush(stdout);
            response = squirrelDB_put((uint8_t *) key, 
                    (uint8_t *) long_len_value, 500);
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
            
            resp_value = (uint8_t *)calloc(sizeof(char), 500);
            response = squirrelDB_get((uint8_t *) key, (uint8_t *) resp_value);
            free(resp_value);
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
            
            rand_string(long_len_value, 500);
            printf("update %s\n", long_len_value);
            fflush(stdout);
            response = squirrelDB_update((uint8_t *)key, 
                    (uint8_t *)long_len_value, 500);
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
            
            response = squirrelDB_delete(key);
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
        }
    } else if(DATA_TYPE == 4) {
        for(i = 100; i < 150; i++) {
            sprintf(key, "key_%d", i);
            
            *num_value = rand();
            response = squirrelDB_put((uint8_t *) key, (uint8_t *) num_value, 
                    sizeof(int));
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
            
            resp_value = (uint8_t *)calloc(sizeof(int), 1);
            response = squirrelDB_get((uint8_t *) key, (uint8_t *) resp_value);
            free(resp_value);
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
            
            *num_value = rand();
            response = squirrelDB_update((uint8_t *)key, (uint8_t *)num_value, 
                    sizeof(int));
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
            
            response = squirrelDB_delete(key);
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
        }
        
        for(i = 150; i < 185; i++) {
            sprintf(key, "key_%d", i);
            
            rand_string(med_len_value, 50);
            response = squirrelDB_put((uint8_t *) key, 
                    (uint8_t *) med_len_value, 50);
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
            
            resp_value = (uint8_t *)calloc(sizeof(char), 50);
            response = squirrelDB_get((uint8_t *) key, (uint8_t *) resp_value);
            free(resp_value);
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
            
            rand_string(med_len_value, 50);
            response = squirrelDB_update((uint8_t *)key, 
                    (uint8_t *)med_len_value, 50);
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
            
            response = squirrelDB_delete(key);
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
        }
        
        for(i = 185; i < 200; i++) {
            sprintf(key, "key_%d", i);
            
            rand_string(long_len_value, 500);
            response = squirrelDB_put((uint8_t *) key, 
                    (uint8_t *) long_len_value, 500);
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
            
            resp_value = (uint8_t *)calloc(sizeof(char), 500);
            response = squirrelDB_get((uint8_t *) key, (uint8_t *) resp_value);
            free(resp_value);
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
            
            rand_string(long_len_value, 500);
            response = squirrelDB_update((uint8_t *)key, 
                    (uint8_t *)long_len_value, 500);
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
            
            response = squirrelDB_delete(key);
            
            if(response == FAILURE) {
                printf("Something went wrong!!!\n");
                fflush(stdout);
                
                squirrelDB_finalize();
                return 0;
            }
        }
    }
    
    printf("Performance Evaluation Over\n");
    fflush(stdout);
    
    printf("---------- PERF. EVAL. OVER ----------\n");
    fflush(stdout);

    squirrelDB_finalize();

    return 1;
}
