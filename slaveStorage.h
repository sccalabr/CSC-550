#ifndef SLAVE_STORAGE_H
#define SLAVE_STORAGE_H

#include "util.h"

#ifndef MEM_BLOCK_SIZE
#define MEM_BLOCK_SIZE 16
#endif

typedef struct _data_block {
    /*  
        Block header is a pointer to the next block containing the next part of
        the data if the data is split across multiple blocks, otherwise it is 
        a timestamp
     */
    uint32_t block_hdr;
    uint8_t data[MEM_BLOCK_SIZE - 4];
} __attribute__((packed)) data_block;

#ifndef MEM_SIZE
#ifdef SMALL_TEST
#define MEM_SIZE 100000
#else 
#define MEM_SIZE 500000000
#endif
#endif

data_block *data;

typedef struct _free_mem free_mem;

struct _free_mem {
    uint32_t mem_loc_index;
    free_mem *next_free_block;
} __attribute__((packed));

/* init when call restore_slave() */
free_mem* free_mem_root;

/* Should find the next free space in free_mem tree, and fill with data */
uint32_t put_slave(uint8_t *value, uint32_t data_len);

/* Should go to data_loc and fill data, which should already be 
 * calloc'd to right size */
uint32_t get_slave(uint32_t data_loc_index, uint32_t data_len, uint8_t *return_data, uint32_t *timestamp);

uint32_t restore_slave();

/* Should put all blocks that were used to store this data in the free list */
int delete_slave(uint32_t data_loc_index, uint32_t data_len);

int flush_slave();

int free_slave_memory();

#endif
