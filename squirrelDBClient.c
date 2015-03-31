#include "squirrelDBClient.h"

int server_fd;

int collect_timing_info = FALSE;

long int avg_put_time = 0;
int num_avg_put = 0;
long int avg_get_time = 0;
int num_avg_get = 0;
long int avg_update_time = 0;
int num_avg_update = 0;
long int avg_delete_time = 0;
int num_avg_delete = 0;

struct timeval tval_before, tval_after, tval_result;

void send_to_db(void *op, int len);
void read_from_db(void *resp, int len);

/* ---------------------------------------------------------------------------*/

/* CLIENT FUNCTIONS */

/* 
 *  SquirrelDB client functions: 
 *      squirrelDB_init() - Try to connect to the port the database is listening
 *          to.  
 *      squirrelDB_put() - Run a put request. 
 *      squirrelDB_get() - Run a get request.
 *      squirrelDB_update() - Run an update request.
 *      squirrelDB_delete() - Run a delete request.
 *      squirrelDB_slave_go_down() - Tell the master to shut down a slave
 *      squirrelDB_master_go_down() - Tell the master to shut down
 *      squirrelDB_close() - Close connection to database.  
 */

int squirrelDB_init(char *addr) {
    struct sockaddr_in server;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_addr.s_addr = inet_addr(addr);
    server.sin_family = AF_INET;
    server.sin_port = htons(8888);

    if (connect(server_fd, (struct sockaddr *) &server, sizeof (server)) < 0) {
        D(printf("Problem connecting to database.\n");)
        return 0;
    }

    D(printf("Connected to database!\n");)
    return 1;
}

int squirrelDB_put(uint8_t *key, uint8_t *value, uint32_t data_len) {

    D(printf("TRYING TO PUT VALUE '%s' WITH KEY '%s' IN DATABASE\n", value, key);)
    D(fflush(stdout);)

    if(collect_timing_info == TRUE) {
        gettimeofday(&tval_before, NULL);
    }

    if (key[15] != 0) {
        return FAILURE;
    }

    op_packet_t op;
    op.op = OP_PUT;
    send_to_db(&op, sizeof (op_packet_t));
    send_to_db(key, 16);
    send_to_db(&data_len, sizeof (uint32_t));

    int resp;
    read_from_db(&resp, sizeof (int));

    if (resp == SUCCESS) {
        send_to_db(value, data_len);

        read_from_db(&resp, sizeof (int));

        if (resp == SUCCESS) {
            D(printf("    SUCCESS, VALUE PUT\n");)
            D(fflush(stdout);)
        } else {
            D(printf("    FAILURE, VALUE NOT PUT\n");)
            D(fflush(stdout);)
        }
    } else {
        D(printf("    FAILURE, VALUE NOT PUT\n");)
        D(fflush(stdout);)
    }

    if(collect_timing_info == TRUE) {
        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);

        long int new_time = (long int) tval_result.tv_sec * 1000000 + 
            (long int) tval_result.tv_usec;
        avg_put_time = ((avg_put_time * num_avg_put) + new_time) / (num_avg_put + 1);
	num_avg_put++;
    }

    return resp;
}

int squirrelDB_get(uint8_t *key, uint8_t *value) {

    D(printf("GETTING VALUE FOR KEY '%s' FROM DATABASE\n", key);)
    D(fflush(stdout);)

    if(collect_timing_info == TRUE) {
        gettimeofday(&tval_before, NULL);
    }

    if (key[15] != 0) {
        return FAILURE;
    }

    op_packet_t op;
    op.op = OP_GET;
    send_to_db(&op, sizeof (op_packet_t));
    send_to_db(key, 16);

    int resp;
    read_from_db(&resp, sizeof (int));

    if (resp == SUCCESS) {
        uint32_t data_len;
        read_from_db(&data_len, sizeof (uint32_t));
        if (value == NULL) {
            value = (uint8_t *) calloc(sizeof (uint8_t), data_len);
        }
       	
	read_from_db(value, data_len);
    }

    if (resp == SUCCESS) {
        D(printf("    SUCCESS, GOT VALUE '%s'\n", value);)
        D(fflush(stdout);)
    } else {
        D(printf("    FAILURE, DID NOT GET VALUE\n");)
        D(fflush(stdout);)
    }
    
    if(collect_timing_info == TRUE) {
        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);

        long int new_time = (long int) tval_result.tv_sec * 1000000 + 
            (long int) tval_result.tv_usec;
        avg_get_time = ((avg_get_time * num_avg_get) + new_time) / (num_avg_get + 1);
    	num_avg_get++;
    }
    
    return resp;
}

int squirrelDB_update(uint8_t *key, uint8_t *value, uint32_t data_len) {

    D(printf("UPDATING KEY '%s' TO VALUE '%s' IN DATABASE\n", key, value);)
    D(fflush(stdout);)

    if(collect_timing_info == TRUE) {
        gettimeofday(&tval_before, NULL);
    }

    if (key[15] != 0) {
        return FAILURE;
    }

    op_packet_t op;
    op.op = OP_UPDATE;
    send_to_db(&op, sizeof (op_packet_t));
    send_to_db(key, 16);
    send_to_db(&data_len, sizeof (uint32_t));
    send_to_db(value, data_len);

    int resp;
    read_from_db(&resp, sizeof (int));

    if (resp == SUCCESS) {
        D(printf("    SUCCESS, VALUE UPDATED\n");)
        D(fflush(stdout);)
    } else {
        D(printf("    FAILURE, VALUE NOT UPDATED\n");)
        D(fflush(stdout);)
    }
    
    if(collect_timing_info == TRUE) {
        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);

        long int new_time = (long int) tval_result.tv_sec * 1000000 + 
            (long int) tval_result.tv_usec;
        avg_update_time = ((avg_update_time * num_avg_update) + new_time) / 
                (num_avg_update + 1);
	num_avg_update++;
    }

    return resp;
}

int squirrelDB_delete(uint8_t *key) {

    D(printf("DELETING VALUE FOR KEY '%s' FROM DATABASE\n", key);)
    D(fflush(stdout);)
       
    if(collect_timing_info == TRUE) {
        gettimeofday(&tval_before, NULL);
    }

    if (key[15] != 0) {
        return FAILURE;
    }

    op_packet_t op;
    op.op = OP_DELETE;
    send_to_db(&op, sizeof (op_packet_t));
    send_to_db(key, 16);

    int resp;
    read_from_db(&resp, sizeof (int));

    if (resp == SUCCESS) {
        D(printf("    SUCCESS, VALUE DELETED\n");)
        D(fflush(stdout);)
    } else {
        D(printf("    FAILURE, VALUE NOT DELETED\n");)
        D(fflush(stdout);)
    }
    
    if(collect_timing_info == TRUE) {
        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);

        long int new_time = (long int) tval_result.tv_sec * 1000000 + 
            (long int) tval_result.tv_usec;
        avg_delete_time = ((avg_delete_time * num_avg_delete) + new_time) / 
                (num_avg_delete + 1);
	num_avg_delete++;
    }

    return resp;
}

int squirrelDB_slave_go_down(uint8_t node) {

    D(printf("TRYING TO SHUT DOWN NODE %d\n", node);)
    D(fflush(stdout);)

    if (node == 0) {
        D(printf("    FAILURE, CANNOT SHUT DOWN NODE 0\n");)
        D(fflush(stdout);)
        return FAILURE;
    }

    op_packet_t op;
    op.op = OP_SLAVE_GO_DOWN;
    send_to_db(&op, sizeof (op_packet_t));
    send_to_db(&node, 1);

    int resp;
    read_from_db(&resp, sizeof (int));

    if (resp == SUCCESS) {
        D(printf("    SUCCESS, NODE SHUT DOWN\n");)
        D(fflush(stdout);)
    } else {
        D(printf("    FAILURE, NODE NOT SHUT DOWN\n");)
        D(fflush(stdout);)
    }

    return resp;
}

int squirrelDB_master_go_down() {
    op_packet_t op;
    op.op = OP_MASTER_GO_DOWN;
    send_to_db(&op, sizeof (op_packet_t));

    int resp;
    read_from_db(&resp, sizeof (int));

    if (resp == SUCCESS) {
        D(printf("    SUCCESS, MASTER SHUT DOWN\n");)
        D(fflush(stdout);)
    } else {
        D(printf("    FAILURE, MASTER NOT SHUT DOWN\n");)
        D(fflush(stdout);)
    }

    if (close(server_fd) == -1) {
        D(printf("ERROR closing connection.\n");)
        D(fflush(stdout);)
    }

    return resp;
}

int squirrelDB_finalize() {
    op_packet_t op;
    op.op = OP_SHUTDOWN;
    send_to_db(&op, sizeof (op_packet_t));

    if (close(server_fd) == -1) {
        D(printf("ERROR finalizing.\n");)
        D(fflush(stdout);)
    }
    
    T(printf("Put Avg Time: %ld microseconds\n", avg_put_time);)
    T(printf("Get Avg Time: %ld microseconds\n", avg_get_time);)
    T(printf("Update Avg Time: %ld microseconds\n", avg_update_time);)
    T(printf("Delete Avg Time: %ld microseconds\n", avg_delete_time);)

    return SUCCESS;
}

void start_collecting() {
    collect_timing_info = TRUE;
}

/* END CLIENT FUNCTIONS */

/* ---------------------------------------------------------------------------*/

/* COMMUNICATION FUNCTIONS */

/* 
 * send_to_db() - writes to port of database
 * read_from_db() - reads from database port
 */

void send_to_db(void *op, int len) {
    if (write(server_fd, op, len) == -1) {
        D(printf("ERROR with connection to database during sending.\n");)
        D(fflush(stdout);)
    }
}

void read_from_db(void *resp, int len) {
    if (read(server_fd, resp, len) == -1) {
        D(printf("ERROR with connection to database during reading.\n");)
        D(fflush(stdout);)
    }
}

/* END COMMUNICATION FUNCTIONS */

/* ---------------------------------------------------------------------------*/
