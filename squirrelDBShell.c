#include "squirrelDBClient.h"
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

/* File should include lines that are of the form: <PUT,GET,UPDATE,DELETE> <KEY> <VALUE> */
int main(int num_args, char **argv) {
    if(num_args != 2) {
        printf("Usuage: squirrelDBShell <filename>\n");
        return 1;
    }
    
    int fd = open(argv[1], O_RDONLY);
    if(fd <= 0) {
        printf("Problem opening file\n");
        return 1;
    }
    
    FILE *fp = fopen(argv[1], "r");
    if(fp == NULL) {
        printf("Problem opening file\n");
        return 1;
    }
    
    squirrelDB_init("127.0.0.1");
    
    char *line = NULL;
    size_t line_cap = 0;
    ssize_t line_len;
    while((line_len = getline(&line, &line_cap, fp)) > 0) {
        char *operation = strtok(line, " ");
        if(strcmp(operation, "PUT") == 0) {
            
            char *tok = strtok(NULL, " ");
            char *key = (char *) calloc(sizeof(char), 16);
            memcpy(key, tok, (strlen(tok) <= 16) ? strlen(tok) : 16);
            
            tok  = strtok(NULL, "\n");
            char *value = (char *)calloc(sizeof(char), strlen(tok));
            memcpy(value, tok, strlen(tok));
            
            
            squirrelDB_put((uint8_t *)key, (uint8_t *)value, strlen(value));
            printf("\n");
        } else if (strcmp(operation, "GET") == 0) {
            char *tok = strtok(NULL, " ");
            char *key = (char *) calloc(sizeof(char), 16);
            memcpy(key, tok, (strlen(tok)-1 <= 16) ? strlen(tok)-1 : 16);
            
            char *value = NULL;
            
            squirrelDB_get((uint8_t *)key, (uint8_t *)value);
            printf("\n");
        } else if (strcmp(operation, "UPDATE") == 0) {
            char *tok = strtok(NULL, " ");
            char *key = (char *) calloc(sizeof(char), 16);
            memcpy(key, tok, (strlen(tok) <= 16) ? strlen(tok) : 16);
            
            tok  = strtok(NULL, "\n");
            char *value = (char *)calloc(sizeof(char), strlen(tok));
            memcpy(value, tok, strlen(tok));
            
            squirrelDB_update((uint8_t *)key, (uint8_t *)value, strlen(value));
            printf("\n");
        } else if (strcmp(operation, "DELETE") == 0) {
            char *tok = strtok(NULL, " ");
            char *key = (char *) calloc(sizeof(char), 16);
            memcpy(key, tok, (strlen(tok)-1 <= 16) ? strlen(tok)-1 : 16);
            
            squirrelDB_delete((uint8_t *)key);
            printf("\n");
        } else {
            printf("Problem parsing shell file, cannot determine action %s\n", operation);
        }
    }
    
    squirrelDB_finalize();
    
    return 0;
}