#ifndef MASTER_STORAGE_H
#define MASTER_STORAGE_H

#include "util.h"

typedef struct _data_loc {
    uint8_t node;
    uint32_t mem_loc_index;
} __attribute__ ((packed)) data_loc ;

#define MAX_KEY_LENGTH 16
#define NUM_DATA_REP_NODES 2

typedef struct _meta_data {
    uint8_t key[MAX_KEY_LENGTH];
    data_loc store_loc[NUM_DATA_REP_NODES];
    uint32_t data_len;
} __attribute__ ((packed)) meta_data;

#ifndef MAX_DATA_TABLE_ENTRIES
#ifdef SMALL_TEST
#define MAX_DATA_TABLE_ENTRIES 12500
#else
#define MAX_DATA_TABLE_ENTRIES 62500000
#endif
#endif

meta_data *meta_data_table;

typedef struct _node_free_mem {
    uint8_t node;
    uint32_t num_free_bytes;
} __attribute__ ((packed)) node_free_mem;

node_free_mem *nodes_free_mem_table;

/* Just building the meta-data table, take an entry, hash off key to get
 * location in table, enter it */
int put_master(meta_data meta_data_for_entry);

/* Just getting the meta-data out of the table, take the key, hash to get entry
 * in meta_data table, set store_loc and data_len to point to values in table */
int get_master(uint8_t key[MAX_KEY_LENGTH], data_loc **store_loc, uint32_t *data_len);

int restore_master(int num_slave_nodes);

/* Hash key to get location in table, remove entry */
int delete_master(uint8_t key[MAX_KEY_LENGTH]);

/* Flush the meta-data table to a file */
/* Should also do some MP to give nodes the file? */
int flush_master();

#endif
