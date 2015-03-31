#include "squirrelDB.h"
// Node ID
int me;

int current_master = 0;

int quit = FALSE;
int finish = FALSE;
int master_went_down = FALSE;

//when heartbeat_count == NUM_HEARTBEATS_TO_FLUSH, flush
int heartbeat_count = 0;

int server_fd;
struct sockaddr_in server_addr;
int client_fd = -1;

// Private Function Declarations
int restore();
void restore_when_master_lost();
void squirrelDB_shutdown();

void loop();
void init_connection();
void master_loop();
void slave_loop();

int master_query_put(uint8_t *key, uint8_t *value, uint32_t data_len,
        int best, int sec_best);
int master_query_get(uint8_t *key, uint8_t *ret_value);
int master_query_delete(uint8_t *key);
int master_query_update(uint8_t *key, uint8_t *value, uint32_t data_len,
        int best, int sec_best);
int check_master_can_store(int data_len, int *best, int *sec_best);

int squirrelDB_comm_sync();
int squirrelDB_send(const void *data, int count, MPI_Datatype datatype,
        int dest, int tag, MPI_Comm comm);
int squirrelDB_recv(void *buf, int count, MPI_Datatype datatype, int source,
        int tag, MPI_Comm comm);

int send_to_client(void *value, int len);
int read_from_client(void *resp, int len);

void print_nodes_free_mem_table();
void print_meta_data_table();

void send_heartbeat_master();
void send_heartbeat_slave();
void setup_heartbeat_alarm_master();
void setup_heartbeat_alarm_slave();
void cancel_heartbeat_alarm_master();
void cancel_heartbeat_alarm_slave();

/* -------------------------------------------------------------------------- */

/* SIGNAL HANDLERS */

void sigint_handler(int signal) {
    if (signal == SIGALRM) {
        if (!heartbeat_alarm) {
            D(printf("    ALARM WENT OFF\n");)

            quit = TRUE;
        } else {
            if (me == current_master) {
                heartbeat_alarm = FALSE;
                send_heartbeat_master();
                setup_heartbeat_alarm_master();
            } else {
                D(printf("NODE %d: MASTER NODE DOWN\n", me);)
                D(fflush(stdout);)

                master_went_down = TRUE;
            }
        }
    }
}

/* END SIGNAL HANDLERS */

/* -------------------------------------------------------------------------- */

/* MAIN */

int main(int num_args, char **args) {

    /* setup signal handler */
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGALRM, &sa, NULL) < 0) {
        perror("Error setting up SIGALRM handler");
        return 2;
    }

    /* initialize MPI */
    MPI_Init(&num_args, &args);
    MPI_Comm_rank(MPI_COMM_WORLD, &me);

    /* get the world group */
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);

    /* duplicate comm world */
    MPI_Comm_dup(MPI_COMM_WORLD, &world_comm);

    /* create a new comm world with only you in the group */
    MPI_Group_incl(world_group, 1, &me, &groups[me]);
    MPI_Comm_create(MPI_COMM_WORLD, groups[me], &comms[me]);

    do {

        if (master_went_down == TRUE) {

            restore_when_master_lost();
            master_went_down = FALSE;
        }

        /* restore each node */
        restore();

        /* enter main loop */
        loop();
    } while (finish == FALSE);

    return 0;
}

/* END MAIN */

/* -------------------------------------------------------------------------- */

/* STARTUP AND SHUTDOWN FUNCTIONS */

/*
    Node startup and shutdown functions:
        restore() -
            Master node will call its restore function and then wait for a 
            message from each slave node telling it how much free space said
            slave node has.
            
            Slave node will call its own restore function and then send its
            available space back to the master.
        
        squirrelDB_shutdown() -
            Master writes all data to disk and then frees all memory
            
            Slave writes all data to disk and then frees all memory
 
 */
int restore() {
    D(printf("NODE %d: HELLO CRUEL WORLD!\n", me);)
    D(fflush(stdout);)

    if (me == current_master) {
        restore_master(NUM_SLAVE_NODES);

        int i, j;
        for (i = 0, j = 0; i < NUM_NODES; i++) {
            if (active_nodes[i] == TRUE && i != current_master) {
                nodes_free_mem_table[j].node = i;
                MPI_Recv(&(nodes_free_mem_table[j++].num_free_bytes), 1,
                        MPI_INT, i, 0, world_comm, NULL);
            }
        }
    } else {
        int available_memory = restore_slave();
        
        printf("%d AVAIL MEM: %d\n", me, available_memory);
        fflush(stdout);

        MPI_Send(&available_memory, 1, MPI_INT, current_master, 0, world_comm);
    }

    return SUCCESS;
}

void restore_when_master_lost() {
    free_slave_memory();

    active_nodes[current_master] = FALSE;
    current_master = current_master + 1;
    NUM_SLAVE_NODES--;

    D(printf("NODE %d: NEW MASTER IS %d\n", me, current_master));
    D(fflush(stdout);)
}

void squirrelDB_shutdown() {
    D(printf("NODE %d: GOODBYE CRUEL WORLD!\n", me);)
    D(fflush(stdout);)

    finish = TRUE;

    MPI_Finalize();
}

/* END STARTUP AND SHUTDOWN FUNCTIONS */

/* -------------------------------------------------------------------------- */

/* LOOP FUNCTIONS */

/*
    Loop run by each node:
        master_loop() - Wait for external query call and then execute 
            appropriate query communication protocol
 
        slave_loop() - Wait for master to send which query call is to be 
            exectuted and then execute appropriate query communication protocol
 
 */

void loop() {
    if (me == current_master) {
        master_loop();
    } else {
        slave_loop();
    }
}

void master_loop() {

    init_connection();

    while (TRUE) {
        setup_heartbeat_alarm_master();

        op_packet_t op;

        int read_amount;
        do {
            read_amount = read_from_client(&op, sizeof (op_packet_t));
        } while (read_amount == 0);

        cancel_heartbeat_alarm_master();

        send_heartbeat_master();

        P(D(print_meta_data_table();))
        P(D(print_nodes_free_mem_table();))
        D(printf("\n");)
        D(printf("/* ------------------------------------------------------------"
                "-------------- */\n");)
        D(fflush(stdout);)       

        if (op.op == OP_SHUTDOWN) {
            int i;
            for (i = 0; i < NUM_NODES; i++) {
                if (i != current_master && active_nodes[i] == TRUE) {
                    MPI_Send(&EXIT, 1, MPI_INT, i, 0, world_comm);
                }
            }

            printf("SquirellDB shutting down\n");
            fflush(stdout);

            squirrelDB_shutdown();
            break;
        } else if (op.op == OP_PUT) {

            uint8_t *key = (uint8_t *) calloc(sizeof (uint8_t), MAX_KEY_LENGTH);

            read_from_client(key, MAX_KEY_LENGTH);

            uint32_t data_len;

            read_from_client(&data_len, 4);

            int best_node, sec_best_node;
            int resp;
            resp = check_master_can_store(data_len, &best_node, &sec_best_node);

            send_to_client(&resp, sizeof (int));

            if (resp == FAILURE) {
                D(printf("NODE %d: put unsuccessful\n", current_master);)
                D(fflush(stdout);)

                continue;
            }

            uint8_t *value = (uint8_t *) calloc(sizeof (uint8_t), data_len);

            read_from_client(value, data_len);

            D(printf("NODE %d: received put('%s', '%s')\n", current_master,
                    key, value);)
                    D(fflush(stdout);)

            if ((resp = master_query_put(key, value, data_len, best_node,
                    sec_best_node)) == SUCCESS) {
                D(printf("NODE %d: put successful\n", current_master);)
                D(fflush(stdout);)
            } else {
                D(printf("NODE %d: put unsuccessful\n", current_master);)
                D(fflush(stdout);)
            }

            send_to_client(&resp, sizeof (int));

            free(value);
            free(key);

        } else if (op.op == OP_GET) {
            uint8_t *key = (uint8_t *) calloc(sizeof (uint8_t), MAX_KEY_LENGTH);

            read_from_client(key, MAX_KEY_LENGTH);

            //get the data_len
            data_loc *store_locs = (data_loc *) calloc(sizeof (data_loc),
                    NUM_DATA_REP_NODES);

            D(printf("NODE %d: received get('%s')\n", current_master, key);)
            D(fflush(stdout);)

                    int resp;
            uint32_t data_len;
            if ((resp = get_master(key, &store_locs, &data_len)) == FAILURE) {
                D(printf("NODE %d: key '%s' not found\n", current_master, key);)
                D(fflush(stdout);)

                send_to_client(&resp, sizeof (int));
            } else {
                uint8_t *value = (uint8_t *) calloc(sizeof (uint8_t), data_len);
                if ((resp = master_query_get(key, value)) == SUCCESS) {
                    D(printf("NODE %d: get successful\n", current_master);)
                    D(fflush(stdout);)

                    send_to_client(&resp, sizeof (int));
                    send_to_client(&data_len, sizeof (uint32_t));
                    send_to_client(value, data_len);
                } else {
                    D(printf("NODE %d: get unsuccessful\n", current_master);)
                    D(fflush(stdout);)

                    send_to_client(&resp, sizeof (int));
                }

                free(value);
            }

            free(key);

        } else if (op.op == OP_UPDATE) {

            uint8_t *key = (uint8_t *) calloc(sizeof (uint8_t), MAX_KEY_LENGTH);

            read_from_client(key, MAX_KEY_LENGTH);

            uint32_t data_len;

            read_from_client(&data_len, 4);

            int best_node, sec_best_node;
            int resp;
            resp = check_master_can_store(data_len, &best_node, &sec_best_node);

            if (resp == FAILURE) {
                D(printf("NODE %d: put unsuccessful\n", current_master);)
                D(fflush(stdout);)

                send_to_client(&resp, sizeof (int));
                continue;
            }

            uint8_t *value = (uint8_t *) calloc(sizeof (uint8_t), data_len);

            read_from_client(value, data_len);

            D(printf("NODE %d: received update('%s, '%s')\n", current_master,
                    key, value);)
                    D(fflush(stdout);)

            if ((resp = master_query_update(key, value, data_len,
                    best_node, sec_best_node)) == SUCCESS) {
                D(printf("NODE %d: update successful\n", current_master);)
                D(fflush(stdout);)
            } else {
                D(printf("NODE %d: update unsuccessful\n", current_master);)
                D(fflush(stdout);)
            }

            send_to_client(&resp, sizeof (int));

            free(value);
            free(key);

        } else if (op.op == OP_DELETE) {

            uint8_t *key = (uint8_t *) calloc(sizeof (uint8_t), MAX_KEY_LENGTH);

            read_from_client(key, MAX_KEY_LENGTH);

            D(printf("NODE %d: received delete('%s')\n", current_master, key);)
            D(fflush(stdout);)

                    int resp;
            if ((resp = master_query_delete(key)) == SUCCESS) {
                D(printf("NODE %d: delete successful\n", current_master);)
                D(fflush(stdout);)
            } else {
                D(printf("NODE %d: delete unsuccessful\n", current_master);)
                D(fflush(stdout);)
            }

            send_to_client(&resp, sizeof (int));

            free(key);

        } else if (op.op == OP_SLAVE_GO_DOWN) {
            uint8_t node;
            read_from_client(&node, 1);

            D(printf("NODE %d: telling node %d to shutdown\n",
                    current_master, node);)
                    D(fflush(stdout);)

                    squirrelDB_send(&STOP_SENDING_RECVING, 1, MPI_INT,
                    node, 0, world_comm);
            squirrelDB_comm_sync();

            int resp = SUCCESS;
            send_to_client(&resp, sizeof (int));

        } else if (op.op == OP_MASTER_GO_DOWN) {
            int resp = SUCCESS;
            send_to_client(&resp, sizeof (int));

            close(client_fd);
            close(server_fd);

            squirrelDB_shutdown();
            break;
        }

        D(fflush(stdout);)
    }
}

void slave_loop() {
    //wait for master to tell you to begin
    int msg;
    MPI_Recv(&msg, 1, MPI_INT, current_master, 0, world_comm, NULL);

    while (TRUE) {
        setup_heartbeat_alarm_slave();

        int msg;
        MPI_Request request;
        MPI_Irecv(&msg, 1, MPI_INT, current_master, 0, world_comm, &request);

        int flag = FALSE;
        while (flag == FALSE && master_went_down == FALSE) {
            MPI_Test(&request, &flag, MPI_STATUS_IGNORE);
        }

        cancel_heartbeat_alarm_slave();

        if (master_went_down == TRUE) {
            break;
        }

        if (msg == PUT) {

            int data_len;
            squirrelDB_recv(&data_len, 1, MPI_INT, current_master,
                    0, world_comm);
            squirrelDB_comm_sync();

            uint8_t *buff = (uint8_t *) calloc(sizeof (uint8_t), data_len);
            squirrelDB_recv(buff, data_len, MPI_BYTE, current_master,
                    0, world_comm);
            squirrelDB_comm_sync();

            int location = put_slave(buff, data_len);

            D(printf("    NODE %d: PUT VALUE '%s' INTO LOCAL MEMORY\n",
                    me, buff);)
                    D(fflush(stdout);)

            squirrelDB_send(&location, 1, MPI_INT, current_master,
                    0, world_comm);

            free(buff);

            squirrelDB_comm_sync();


        } else if (msg == GET) {
            int mem_loc_index;
            squirrelDB_recv(&mem_loc_index, 1, MPI_INT, current_master,
                    0, world_comm);
            squirrelDB_comm_sync();

            int data_len;
            squirrelDB_recv(&data_len, 1, MPI_INT, current_master,
                    0, world_comm);
            squirrelDB_comm_sync();

            uint8_t *buff = (uint8_t *) calloc(sizeof (uint8_t), data_len);
            uint32_t timestamp = 0;
            get_slave(mem_loc_index, data_len, buff, &timestamp);

            D(printf("    NODE %d: GOT VALUE FROM LOCAL MEMORY WITH "
                    "TIMESTAMP %u\n", me, timestamp);)
                    D(fflush(stdout);)

            squirrelDB_send(buff, data_len, MPI_BYTE, current_master,
                    0, world_comm);
            squirrelDB_send(&timestamp, 1, MPI_INT, current_master,
                    0, world_comm);
            squirrelDB_comm_sync();

            free(buff);

        } else if (msg == DELETE) {

            int mem_loc_index;
            squirrelDB_recv(&mem_loc_index, 1, MPI_INT, current_master,
                    0, world_comm);
            squirrelDB_comm_sync();

            int data_len;
            squirrelDB_recv(&data_len, 1, MPI_INT, current_master,
                    0, world_comm);
            squirrelDB_comm_sync();

            D(printf("    NODE %d: DELETING VALUE FROM LOCAL MEMORY\n", me);)
            D(fflush(stdout);)

            int response = delete_slave(mem_loc_index, data_len);

            squirrelDB_send(&response, 1, MPI_INT, current_master,
                    0, world_comm);
            squirrelDB_comm_sync();


        } else if (msg == STOP_SENDING_RECVING) {

            D(printf("    NODE %d: SHUTTING DOWN\n", me);)
            D(fflush(stdout);)

            squirrelDB_shutdown();
            break;
        } else if (msg == EXIT) {
            squirrelDB_shutdown();
            break;
        } else if (msg == HEARTBEAT) {
            send_heartbeat_slave();
        }
    }
}

/* END LOOP FUNCTIONS */

/* -------------------------------------------------------------------------- */

/* MASTER QUERY FUNCTIONS */

/*
    Internal query functions:
        master_query_put() - Master has found two nodes to store the data. 
            Maser tells the chosen slave nodes to store the data. Master then 
            waits for slave to respond with where the data is stored. Master 
            then adds entry to meta-data table and updates the free memory 
            table.
 
        master_query_get() - Master finds which nodes have the data. Then master
            asks for the data. Finally master returns the freshest data
 
        master_query_delete() - Master finds which nodes have the data. Then 
            master tells them to delete data. After they respond that they have 
            deleted the data, master removes key from meta-data table and 
            updates free memory table.
 
        master_query_update() -  Master finds out who currently has the data. 
            Then master tells the slaves who have it to delete it. Finally 
            master performs put function.
 */

int master_query_put(uint8_t *key, uint8_t *value, uint32_t data_len,
        int best, int sec_best) {
    meta_data put_meta_data;
    memcpy(put_meta_data.key, key, MAX_KEY_LENGTH);
    put_meta_data.data_len = data_len;

    //make sure key isn't already in database
    data_loc **temp = (data_loc **) calloc(sizeof (data_loc *), 1);
    int resp = get_master(key, temp, &data_len);
    free(temp);

    //get_master should have returned failure
    if (resp == SUCCESS) {
        return FAILURE;
    }

    squirrelDB_send(&PUT, 1, MPI_INT, best, 0, world_comm);
    squirrelDB_send(&PUT, 1, MPI_INT, sec_best, 0, world_comm);

    squirrelDB_send(&data_len, 1, MPI_INT, best, 0, world_comm);
    squirrelDB_send(&data_len, 1, MPI_INT, sec_best, 0, world_comm);

    squirrelDB_send(value, data_len, MPI_BYTE, best, 0, world_comm);
    squirrelDB_send(value, data_len, MPI_BYTE, sec_best, 0, world_comm);

    squirrelDB_comm_sync();

    // record where value was stored for the meta-data table
    put_meta_data.store_loc[0].node = (uint8_t) best;
    put_meta_data.store_loc[1].node = (uint8_t) sec_best;

    //recieve back from the slave nodes the index of the fist block that they
    //store value in
    squirrelDB_recv(&(put_meta_data.store_loc[0].mem_loc_index), 1, MPI_INT,
            best, 0, world_comm);
    squirrelDB_comm_sync();
    squirrelDB_recv(&(put_meta_data.store_loc[1].mem_loc_index), 1, MPI_INT,
            sec_best, 0, world_comm);
    squirrelDB_comm_sync();

    //put the meta-data in the meta-data table
    resp = put_master(put_meta_data);

    if (resp == FAILURE) {
        return FAILURE;
    }

    int i;
    //update the free memory for the nodes that just stored value
    for (i = 0; i < NUM_SLAVE_NODES && active_nodes[i] == TRUE; i++) {
        if (nodes_free_mem_table[i].node == best ||
                nodes_free_mem_table[i].node == sec_best) {
            nodes_free_mem_table[i].num_free_bytes -=
                    (data_len % (MEM_BLOCK_SIZE - 4) == 0) ?
                    (data_len / (MEM_BLOCK_SIZE - 4)) * MEM_BLOCK_SIZE :
                    (data_len / (MEM_BLOCK_SIZE - 4) + 1) * MEM_BLOCK_SIZE;
        }
    }

    return SUCCESS;
}

int master_query_get(uint8_t *key, uint8_t *ret_value) {
    data_loc *store_locs = (data_loc *) calloc(sizeof (data_loc),
            NUM_DATA_REP_NODES);

    uint32_t data_len;
    //get which nodes stored the data from the meta-data table
    int resp = get_master(key, &store_locs, &data_len);

    if (resp == FAILURE) {
        return FAILURE;
    }

    //Ask each node that has the data for the data
    uint8_t** buffs = (uint8_t **) calloc(sizeof (uint8_t *),
            NUM_DATA_REP_NODES);
    uint32_t* timestamps = (uint32_t *) calloc(sizeof (uint32_t),
            NUM_DATA_REP_NODES);

    int i;
    for (i = 0; i < NUM_DATA_REP_NODES; i++) {
        if(store_locs[i].node != current_master && 
                active_nodes[store_locs[i].node] == TRUE) {
            squirrelDB_send(&GET, 1, MPI_INT, store_locs[i].node, 0, world_comm);
        }
    }

    for (i = 0; i < NUM_DATA_REP_NODES; i++) {
        if(store_locs[i].node != current_master && 
                active_nodes[store_locs[i].node] == TRUE) {
            squirrelDB_send(&(store_locs[i].mem_loc_index), 1, MPI_INT,
                store_locs[i].node, 0, world_comm);
        }
    }

    for (i = 0; i < NUM_DATA_REP_NODES; i++) {
        if(store_locs[i].node != current_master && 
                active_nodes[store_locs[i].node] == TRUE) {
            squirrelDB_send(&data_len, 1, MPI_INT, store_locs[i].node,
                0, world_comm);
        }
    }

    squirrelDB_comm_sync();

    for (i = 0; i < NUM_DATA_REP_NODES; i++) {
        buffs[i] = (uint8_t *) calloc(sizeof (uint8_t), data_len);
        if(store_locs[i].node != current_master && 
                active_nodes[store_locs[i].node] == TRUE) {
            squirrelDB_recv(buffs[i], data_len, MPI_BYTE, store_locs[i].node, 0,
                    world_comm);
            squirrelDB_comm_sync();

            squirrelDB_recv(&(timestamps[i]), 1, MPI_INT, store_locs[i].node,
                    0, world_comm);
            squirrelDB_comm_sync();
        }
    }

    uint32_t largest_timestamp = 0;
    int index = -1;

    for (i = 0; i < NUM_DATA_REP_NODES; i++) {
        if (timestamps[i] > largest_timestamp) {
            largest_timestamp = timestamps[i];
            index = i;
        }
    }
    
    if(largest_timestamp > 0) {

        D(printf("        RETURNING DATA FROM NODE %d\n", store_locs[index].node);)
        D(fflush(stdout);)

        memcpy(ret_value, buffs[index], data_len);

        for (i = 0; i < NUM_DATA_REP_NODES; i++) {
            free(buffs[i]);
        }

        free(buffs);

        return SUCCESS;
    } else {
        return FAILURE;
    }
}

int master_query_delete(uint8_t *key) {
    data_loc *store_locs = (data_loc *) calloc(sizeof (data_loc),
            NUM_DATA_REP_NODES);
    uint32_t data_len;

    //Find out who stored the value
    int resp = get_master(key, &store_locs, &data_len);

    if (resp == FAILURE) {
        return FAILURE;
    }

    //Tell each node that stored the value to delete it
    int i;
    for (i = 0; i < NUM_DATA_REP_NODES; i++) {
        if(store_locs[i].node != current_master && 
                active_nodes[store_locs[i].node] == TRUE) {
            squirrelDB_send(&DELETE, 1, MPI_INT, store_locs[i].node, 0, 
                    world_comm);
        }
    }

    for (i = 0; i < NUM_DATA_REP_NODES; i++) {
        if(store_locs[i].node != current_master && 
                active_nodes[store_locs[i].node] == TRUE) {
            squirrelDB_send(&(store_locs[i].mem_loc_index), 1, MPI_INT,
                store_locs[i].node, 0, world_comm);
        }
    }

    for (i = 0; i < NUM_DATA_REP_NODES; i++) {
        if(store_locs[i].node != current_master && 
                active_nodes[store_locs[i].node] == TRUE) {
            squirrelDB_send(&data_len, 1, MPI_INT, store_locs[i].node, 0,
                world_comm);
        }
    }

    squirrelDB_comm_sync();

    for (i = 0; i < NUM_DATA_REP_NODES; i++) {
        int response;
        if(store_locs[i].node != current_master && 
                active_nodes[store_locs[i].node] == TRUE) {
            squirrelDB_recv(&response, 1, MPI_INT, store_locs[i].node, 0,
                    world_comm);
            squirrelDB_comm_sync();
        }
    }

    //update free memory table
    for (i = 0; i < NUM_SLAVE_NODES && active_nodes[i] == TRUE; i++) {
        if ((nodes_free_mem_table[i].node == store_locs[0].node) ||
                (nodes_free_mem_table[i].node == store_locs[1].node)) {
            nodes_free_mem_table[i].num_free_bytes +=
                    (data_len % (MEM_BLOCK_SIZE - 4) == 0) ?
                    (data_len / (MEM_BLOCK_SIZE - 4)) * MEM_BLOCK_SIZE :
                    (data_len / (MEM_BLOCK_SIZE - 4) + 1) * MEM_BLOCK_SIZE;
            ;
        }
    }

    //delete key from meta-data table
    delete_master(key);

    return SUCCESS;
}

int master_query_update(uint8_t *key, uint8_t *value, uint32_t data_len,
        int best, int sec_best) {
    int resp;
    data_loc *store_locs = (data_loc *) calloc(sizeof (data_loc),
            NUM_DATA_REP_NODES);

    uint32_t data_len_old;
    //get which nodes stored the data from the meta-data table
    resp = get_master(key, &store_locs, &data_len_old);

    if (resp == FAILURE) {
        return FAILURE;
    }

    resp = master_query_delete(key);
    if (resp == FAILURE) {
        return FAILURE;
    }

    resp = master_query_put(key, value, data_len, best, sec_best);
    if (resp == FAILURE) {
        return FAILURE;
    }

    return SUCCESS;
}

int check_master_can_store(int data_len, int *best, int *sec_best) {
    // Find two nodes with the most free space out of all the slave nodes
    int best_node = -1, sec_best_node = -1;
    int best_free_mem = -1, sec_best_free_mem = -1;
    int i, j = 0;
    for (i = 0; i < NUM_NODES; i++) {
        if (active_nodes[i] && i != current_master) {
            int free_mem = nodes_free_mem_table[j].num_free_bytes;
            int node = nodes_free_mem_table[j++].node;

            if (free_mem > best_free_mem) {
                sec_best_node = best_node;
                sec_best_free_mem = best_free_mem;

                best_node = node;
                best_free_mem = free_mem;
            } else if (free_mem > sec_best_free_mem) {
                sec_best_node = node;
                sec_best_free_mem = free_mem;
            }
        }
    }

    if (data_len > best_free_mem || data_len > sec_best_free_mem) {
        D(printf("NOT ENOUGH FREE MEMORY\n");)
        D(fflush(stdout);)
        return FAILURE;
    }

    if (best_node == -1 || sec_best_node == -1) {
        D(printf("NOT ENOUGH NODES TO REPLICATE\n");)
        D(fflush(stdout);)
        return FAILURE;
    }

    *best = best_node;
    *sec_best = sec_best_node;

    return SUCCESS;
}

/* END MASTER QUERY FUNCTIONS */

/* -------------------------------------------------------------------------- */

/* COMMUNICATION FUNCTIONS */

/*
    Squirrel DB communication functions
        init_connection() - sets up port master node will listen to
 
        squirrelDB_comm_sync() - sync all the pending requests
 
        squirrelDB_send() - nonblocking send to node

        squirrelDB_recv() - nonblocking recv from node
 
        send_to_client() - writes to client
 
        read_from_client() - reads from client
 */

void init_connection() {
    /* setup socket */
    server_fd = socket(PF_INET, SOCK_STREAM, 0);

    memset(&server_addr, 0, sizeof (server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8888);

    int opt1 = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt1,
            sizeof (opt1)) == -1) {
        printf("%s\n", strerror(errno));
        printf("Error setting socket option SO_REUSEADDR.\n");
        exit(-1);
    }

    if (bind(server_fd, (struct sockaddr *) &server_addr,
            sizeof (server_addr)) != 0) {
        printf("%s\n", strerror(errno));
        printf("Error binding TCP socket: Can't assign requested address\n");
        exit(-1);
    }

    uint len = sizeof (struct sockaddr_in);
    if (getsockname(server_fd, (struct sockaddr *) &server_addr, &len) < 0) {
        printf("Error getting socket name\n");
        exit(-1);
    }

    printf("SquirrelDB is up and listening for requests on port %d\n",
            ntohs(server_addr.sin_port));
    fflush(stdout);

    listen(server_fd, 128);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof (addr));
    socklen_t addr_len = sizeof (addr);

    do {
        client_fd = accept(server_fd, (struct sockaddr *) &addr, &addr_len);
    } while (client_fd == -1);

    int i;
    for (i = 0; i < NUM_NODES; i++) {
        if (i != me && active_nodes[i] == TRUE) {
            //tell slaves you are ready to begin
            MPI_Send(&HEARTBEAT, 1, MPI_INT, i, 0, world_comm);
        }
    }
}

int squirrelDB_comm_sync() {
    pending_request *curr_pending_request = get_head();

    my_alarm((long int) PEND_REQ_INTERVAL_USEC, (long int) PEND_REQ_INTERVAL_SEC);

    while (get_head() != NULL && !quit) {
        int flag;

        MPI_Test(curr_pending_request->request, &flag, MPI_STATUS_IGNORE);
        if (flag) {
            pending_request *next_pending_request = curr_pending_request->next;
            remove_pending_request(curr_pending_request);
            curr_pending_request = next_pending_request;
        } else {
            if (curr_pending_request->next == NULL) {
                curr_pending_request = get_head();
            } else {
                curr_pending_request = curr_pending_request->next;
            }
        }
    }

    my_alarm(0, 0);

    if (quit) {
        quit = FALSE;

        curr_pending_request = get_end();

        // all pending are removed and nodes are set inactive
        while (curr_pending_request != NULL) {
            MPI_Cancel(curr_pending_request->request);
            D(printf("        SETTING NODE %d TO NOT ACTIVE\n",
                    curr_pending_request->node);)
                    D(fflush(stdout);)
                    active_nodes[curr_pending_request->node] = FALSE;
            remove_pending_request(curr_pending_request);
            curr_pending_request = get_end();
        }
    }

    return SUCCESS;
}

int squirrelDB_send(const void *data, int count, MPI_Datatype datatype,
        int dest, int tag, MPI_Comm comm) {

    if (active_nodes[dest] && dest != me) {
        MPI_Request *request = (MPI_Request *) calloc(sizeof (MPI_Request), 1);
        MPI_Isend(data, count, datatype, dest, tag, comm, request);

        add_pending_request(dest, request);

        return SUCCESS;
    } else {
        return FAILURE;
    }
}

int squirrelDB_recv(void *buf, int count, MPI_Datatype datatype, int source,
        int tag, MPI_Comm comm) {

    if (active_nodes[source] && source != me) {
        MPI_Request *request = (MPI_Request *) calloc(sizeof (MPI_Request), 1);
        MPI_Irecv(buf, count, datatype, source, tag, comm, request);

        add_pending_request(source, request);

        return SUCCESS;
    } else {
        return FAILURE;
    }
}

int send_to_client(void *op, int len) {
    int wrote;
    if ((wrote = write(client_fd, op, len)) == -1) {
        return FAILURE;
    }

    return wrote;
}

int read_from_client(void *resp, int len) {
    int amount_read;
    if ((amount_read = read(client_fd, resp, len)) == -1) {
        return FAILURE;
    }

    return amount_read;
}

/* END COMMUNICATION FUNCTIONS */

/* -------------------------------------------------------------------------- */

/* PRINT FUNCTIONS */

/*
    Print Functions
        print_nodes_free_mem_table() - Prints free memory each node has
 
        print_meta_data_table() - Prints all entries in the meta-data table that
            currently store data
 */

void print_nodes_free_mem_table() {
    if (me == current_master) {
        D(printf("NODE FREE MEM TABLE\n");)
        D(fflush(stdout);)

                int i;
        for (i = 0; i < NUM_SLAVE_NODES; i++) {
            D(printf("Node %d - %s\n", nodes_free_mem_table[i].node,
                    active_nodes[nodes_free_mem_table[i].node] == TRUE ?
                    "ACTIVE" : "NOT ACTIVE");)
                    D(printf("    Num Free Bytes: %d\n",
                    nodes_free_mem_table[i].num_free_bytes);)
                    D(fflush(stdout);)
        }
    }
}

void print_meta_data_table() {
    if (me == current_master) {
        D(printf("META DATA TABLE (if nothing prints then table empty)\n");)
        D(fflush(stdout);)

                int i;
        for (i = 0; i < MAX_DATA_TABLE_ENTRIES; i++) {
            if (meta_data_table[i].data_len != 0) {
                D(printf("Entry %d:\n", i);)
                D(printf("    Key: %s\n", (char *) meta_data_table[i].key);)
                D(printf("    Store locs:\n");)
                D(printf("        Node: %d\n",
                        meta_data_table[i].store_loc[0].node);)
                        D(printf("            Mem Loc Index: %d\n",
                        meta_data_table[i].store_loc[0].mem_loc_index);)
                        D(printf("        Node: %d\n",
                        meta_data_table[i].store_loc[1].node);)
                        D(printf("            Mem Loc Index: %d\n",
                        meta_data_table[i].store_loc[1].mem_loc_index);)
                        D(printf("    Data Length: %u\n", meta_data_table[i].data_len);)
                        D(fflush(stdout);)
            }
        }

        D(printf("DONE PRINTING META DATA TABLE\n");)
        D(fflush(stdout);)
    }
}

/* END PRINT FUNCTIONS */

/* -------------------------------------------------------------------------- */

/* HEARTBEAT ALARM FUNCTIONS */

/*
 *  Heartbeat functions 
 *      send_heartbeat_master() - master node sends heartbeat message to all
 *          active slaves and waits for them to respond
 * 
 *      send_heartbeat_slave() - slave sends heartbeat message to master
 * 
 *      setup_heartbeat_alarm_master() - sets up heartbeat alarm for master
 * 
 *      setup_heartbeat_alarm_slave() - sets up heartbeat alarm for slave
 * 
 *      cancel_heartbeat_alarm_master() - cancels heartbeat alarm for master
 * 
 *      cancel_heartbeat_alarm_slave() - cancels heartbeat alarm for slave
 */

void send_heartbeat_master() {

    heartbeat_count++;

    int i;
    for (i = 0; i < NUM_NODES; i++) {
        if (active_nodes[i] && me != i) {
            squirrelDB_send(&HEARTBEAT, 1, MPI_INT, i, 0, world_comm);
        }
    }

    squirrelDB_comm_sync();

    int resp;
    for (i = 0; i < NUM_NODES; i++) {
        if (active_nodes[i] && me != i) {
            squirrelDB_recv(&resp, 1, MPI_INT, i, 0, world_comm);
            squirrelDB_comm_sync();
        }
    }

    if (heartbeat_count == NUM_HEARTBEATS_TO_FLUSH) {
        printf("%d FLUSHING\n", me);
        fflush(stdout);

        heartbeat_count = 0;
        flush_master();

        int i;
        for (i = 0; i < NUM_NODES; i++) {
            if (active_nodes[i] && i != current_master) {
                int num_entries = MAX_DATA_TABLE_ENTRIES;
                MPI_Send(&num_entries, 1, MPI_INT, i, 0, world_comm);
            }
        }

        int j;
        for (j = 0; j < MAX_DATA_TABLE_ENTRIES; j++) {
            for (i = 0; i < NUM_NODES; i++) {
                if (active_nodes[i] && i != current_master) {
                    MPI_Send(&(meta_data_table[j]), sizeof(meta_data), MPI_BYTE, i, 0, world_comm);
                }
            }
        }

        printf("%d FINISHED FLUSHING\n", me);
        fflush(stdout);
    }
}

void send_heartbeat_slave() {
    heartbeat_count++;

    squirrelDB_send(&HEARTBEAT, 1, MPI_INT, current_master, 0, world_comm);

    squirrelDB_comm_sync();

    if (heartbeat_count == NUM_HEARTBEATS_TO_FLUSH) {
        printf("%d FLUSHING\n", me);
        fflush(stdout);

        heartbeat_count = 0;
        flush_slave();

        int num_entries;
        MPI_Recv(&num_entries, 1, MPI_INT, current_master, 0, world_comm, NULL);

        FILE *meta_data_fp;
        meta_data_fp = fopen("meta_data.sqdb", "w");

        int i;
        uint8_t *buff = (uint8_t *) calloc(sizeof (meta_data), 1);
        for (i = 0; i < num_entries; i++) {
            MPI_Recv(buff, sizeof(meta_data), MPI_BYTE, current_master, 0, world_comm, NULL);
            fwrite(buff, 1, sizeof (meta_data), meta_data_fp);
        }

        fclose(meta_data_fp);

        printf("%d FINISHED FLUSHING\n", me);
        fflush(stdout);
    }
}

void setup_heartbeat_alarm_master() {
    heartbeat_alarm = TRUE;
    my_alarm(0, (long int) HEARTBEAT_REQ_INTERVAL_MASTER_SEC);
}

void cancel_heartbeat_alarm_master() {
    heartbeat_alarm = FALSE;
    my_alarm(0, 0);
}

void setup_heartbeat_alarm_slave() {
    heartbeat_alarm = TRUE;
    my_alarm(0, (long int) HEARTBEAT_REQ_INTERVAL_SLAVE_SEC);
}

void cancel_heartbeat_alarm_slave() {
    heartbeat_alarm = FALSE;
    my_alarm(0, 0);
}

/* END HEARTBEAT ALARM FUNCTIONS */

/* -------------------------------------------------------------------------- */
