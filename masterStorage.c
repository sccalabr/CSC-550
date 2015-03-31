#include "masterStorage.h"

const uint32_t hash_seed = 12345;

int findIndex(uint32_t hash, uint8_t key[MAX_KEY_LENGTH]);
int findFreeIndex(uint32_t hash, uint8_t key[MAX_KEY_LENGTH]);

/* Just building the meta-data table, take an entry, hash off key to get
 * location in table, enter it */
int put_master(meta_data meta_data_for_entry) {
    uint32_t hash;
    MurmurHash3_x86_32(meta_data_for_entry.key, MAX_KEY_LENGTH, hash_seed, &hash);

    hash = hash % MAX_DATA_TABLE_ENTRIES;

    int index = findFreeIndex(hash, meta_data_for_entry.key);

    if (index == -1) {
        return FAILURE;
    }

    memcpy(&(meta_data_table[index]), &meta_data_for_entry, sizeof (meta_data));

    return SUCCESS;
}

/* Just getting the meta-data out of the table, take the key, hash to get entry
 * in meta_data table, set store_loc and data_len to point to values in table */
int get_master(uint8_t key[MAX_KEY_LENGTH], data_loc **store_loc, uint32_t *data_len) {
    uint32_t hash;
    MurmurHash3_x86_32(key, MAX_KEY_LENGTH, hash_seed, &hash);

    hash = hash % MAX_DATA_TABLE_ENTRIES;

    int index = findIndex(hash, key);

    if (index == -1) {
        return FAILURE;
    }

    *store_loc = meta_data_table[index].store_loc;
    *data_len = meta_data_table[index].data_len;

    return SUCCESS;
}

int restore_master(int num_slave_nodes) {
    meta_data_table = (meta_data *) calloc(sizeof (meta_data), 
            MAX_DATA_TABLE_ENTRIES);
    nodes_free_mem_table = (node_free_mem *) calloc(sizeof (node_free_mem), 
            num_slave_nodes);
    
    FILE *meta_data_fp;
    meta_data_fp = fopen("meta_data.sqdb", "r");
    
    if(meta_data_fp != NULL) {
        printf("RESTORING FROM FILES\n");
        fflush(stdout);
        
        int i;
        for (i = 0; i < MAX_DATA_TABLE_ENTRIES; i++) {
            if (fread(&(meta_data_table[i]), 1, sizeof (meta_data), meta_data_fp) != sizeof (meta_data)) {
                fclose(meta_data_fp);
                return FAILURE;
            }
        }
        
        fclose(meta_data_fp);
        
        printf("RESTORE SUCCESSFUL\n");
        fflush(stdout);
    }

    return SUCCESS;
}

/* Hash key to get location in table, remove entry */
int delete_master(uint8_t key[MAX_KEY_LENGTH]) {
    uint32_t hash;
    MurmurHash3_x86_32(key, MAX_KEY_LENGTH, hash_seed, &hash);

    hash = hash % MAX_DATA_TABLE_ENTRIES;

    int index = findIndex(hash, key);

    if (index == -1) {
        return FAILURE;
    }

    memset(&meta_data_table[index], 0, sizeof (meta_data));
    return SUCCESS;
}

int flush_master() {
    FILE *meta_data_fp;
    meta_data_fp = fopen("meta_data.sqdb", "w");
    
    if(meta_data_fp == NULL) {
        return FAILURE;
    }
    
    int i;
    for (i = 0; i < MAX_DATA_TABLE_ENTRIES; i++) {
        if (fwrite(&(meta_data_table[i]), 1, sizeof (meta_data), meta_data_fp) != sizeof (meta_data)) {
            fclose(meta_data_fp);
            return FAILURE;
        }
    }
    
    fclose(meta_data_fp);

    return SUCCESS;
}

int findIndex(uint32_t hash, uint8_t key[MAX_KEY_LENGTH]) {
    int i = hash;
    do {
        //when key was put it in, it would have been put at hash, making data_len
        //non-zero. If a collision happened then it would have been put at the
        //next free spot. In that case all table entries from hash to wherever
        //key was stored would have non-zero data_len. So if you find an entry
        //with data_len 0 then you can stop looking, because you won't find it.
        if(meta_data_table[i].data_len == 0) {
            return -1;
        }

        if (memcmp(meta_data_table[i].key, key, MAX_KEY_LENGTH) == 0) {
            return i;
        }

        i = (i + 1) % MAX_DATA_TABLE_ENTRIES;

    } while (i != hash);

    return -1;
}

int findFreeIndex(uint32_t hash, uint8_t key[MAX_KEY_LENGTH]) {

    int i = hash;
    do {
        if(meta_data_table[i].data_len == 0) {
            return i;
        } else if (memcmp(meta_data_table[i].key, key, MAX_KEY_LENGTH) == 0) {
            return -1;
        }

        i = (i + 1) % MAX_DATA_TABLE_ENTRIES;

    } while (i != hash);

    return -1;
}
