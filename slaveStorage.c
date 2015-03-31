#include "slaveStorage.h"

/* Should find the next free space in free_mem tree, and fill with data */
uint32_t put_slave(uint8_t *value, uint32_t data_len) {
    free_mem *current = free_mem_root;
    uint32_t location;

    data_block *block_data = &(data[current->mem_loc_index]);

    if ((MEM_BLOCK_SIZE - 4) >= data_len) {
        //will calculate time here once the proper format is determined.
        time((time_t *)&(block_data->block_hdr));
        memcpy(block_data->data, value, data_len);
        free_mem_root = current->next_free_block;
        location = current->mem_loc_index;
        free(current);
        return location;
    } else {
        memcpy(block_data->data, value, MEM_BLOCK_SIZE - 4);
        free_mem_root = (current->next_free_block);
        location = current->mem_loc_index;
        block_data->block_hdr = put_slave(value + (MEM_BLOCK_SIZE - 4), 
                data_len - (MEM_BLOCK_SIZE - 4));
        free(current);
        return location;
    }
}

/* Should go to data_loc and fill data, which should already be
 * calloc'd to right size */
uint32_t get_slave(uint32_t data_loc_index, uint32_t data_len, 
        uint8_t *return_data, uint32_t *timestamp) {
    data_block *current = &(data[data_loc_index]);

    if (MEM_BLOCK_SIZE - 4 >= data_len) {
        memcpy(return_data, current->data, data_len);
        *timestamp = current->block_hdr;
        return data_len;
    } else {
        memcpy(return_data, current->data, MEM_BLOCK_SIZE - 4);
        return (MEM_BLOCK_SIZE - 4) +get_slave(current->block_hdr, data_len -
                (MEM_BLOCK_SIZE - 4), return_data + (MEM_BLOCK_SIZE - 4), 
                timestamp);
    }

}

/* Should setup free_mem table, read from disk backup file if exists
 * return amount of free memory, otherwise 0 if error */
uint32_t restore_slave() {
    data = (data_block *) calloc(sizeof (data_block), MEM_SIZE / MEM_BLOCK_SIZE);

    free_mem  *current;
    uint32_t avail_mem = 0;

    FILE *data_fp, *free_mem_fp;
    data_fp = fopen("data.sqdb", "r");
    free_mem_fp = fopen("free_mem.sqdb", "r");

    if (data_fp != NULL && free_mem_fp != NULL) {
        D(printf("RESTORING FROM FILES\n");)
        D(fflush(stdout);)
        
        int i;
        for (i = 0; i < (MEM_SIZE / MEM_BLOCK_SIZE); i++) {
            if (fread(&(data[i]), 1, sizeof (data_block), data_fp) 
                    != sizeof (data_block)) {
                fclose(data_fp);
                return FAILURE;
            }
        }

	uint32_t mem_loc_index;
        free_mem_root = (free_mem *)calloc(sizeof (free_mem), 1);
        current = free_mem_root;
        free_mem *prev = NULL;
        while (fread(&mem_loc_index, 1, sizeof(uint32_t), free_mem_fp) != 0) {
            current->mem_loc_index = mem_loc_index;
            current->next_free_block = NULL;
            
            if(prev != NULL) {
                prev->next_free_block = current;
            }
            
            prev = current;
            
            current = (free_mem *)calloc(sizeof (free_mem), 1);
            
            avail_mem += (MEM_BLOCK_SIZE - 4);
        }
        
        fclose(data_fp);
        fclose(free_mem_fp);
        
        D(printf("RESTORE SUCCESSFUL\n");)
        D(fflush(stdout);)
    } else {
        free_mem *next;
        int count = 0;
        avail_mem = (MEM_SIZE / MEM_BLOCK_SIZE) * (MEM_BLOCK_SIZE - 4);
        
        free_mem_root = calloc(sizeof (free_mem), 1);
        current = free_mem_root;
        current->mem_loc_index = count;

        for (count = 1; count < (MEM_SIZE / MEM_BLOCK_SIZE); count++) {
            next = calloc(sizeof (free_mem), 1);
            next->mem_loc_index = count;
            next->next_free_block = NULL;
            current->next_free_block = next;
            current = next;
        }
    }

    return avail_mem;
}

/* Should put all blocks that were used to store this data in the free list */
int delete_slave(uint32_t data_loc_index, uint32_t data_len) {
    free_mem* current = malloc(sizeof (free_mem));

    current->mem_loc_index = data_loc_index;
    current->next_free_block = free_mem_root;
    free_mem_root = current;

    if (MEM_BLOCK_SIZE - 4 < data_len) {
        return delete_slave(data[data_loc_index].block_hdr,
                data_len - (MEM_BLOCK_SIZE - 4));
    }

    return SUCCESS;

}

int flush_slave() {
    FILE *data_fp;
    data_fp = fopen("data.sqdb", "w");

    if (data_fp == NULL) {
        return FAILURE;
    }

//    printf("FILE '%s' open\n", "data.sqdb");
//    fflush(stdout);

    int i;
    for (i = 0; i < (MEM_SIZE / MEM_BLOCK_SIZE); i++) {
        if (fwrite(&(data[i]), 1, sizeof (data_block), data_fp) != 
                sizeof (data_block)) {
            fclose(data_fp);
	    printf("FAILED!!!!\n");
	    fflush(stdout);
            return FAILURE;
        }
    }

//    printf("DATA FILE SUCCESSFULLY OUTPUT\n");
//    fflush(stdout);

    fclose(data_fp);

    FILE *free_mem_fp;
    free_mem_fp = fopen("free_mem.sqdb", "w");

    if (free_mem_fp == NULL) {
        return FAILURE;
    }

//    printf("FILE '%s' open\n", "free_mem.sqdb");
//    fflush(stdout);

    free_mem *current = free_mem_root;

    while (current != NULL) {
        if (fwrite(&(current->mem_loc_index), 1, sizeof (uint32_t), free_mem_fp)
                != sizeof (uint32_t)) {
            fclose(free_mem_fp);
            return FAILURE;
        }

        current = current->next_free_block;
    }

//    printf("FREE MEM FILE SUCCESSFULLY OUTPUT\n");
//    fflush(stdout);
    
    fclose(free_mem_fp);

    return SUCCESS;
}

int free_slave_memory() {
    free(data);

    free_mem *current = free_mem_root;
    while (current != NULL) {
        free_mem *next = current->next_free_block;
        free(current);
        current = next;
    }

    return SUCCESS;
}
