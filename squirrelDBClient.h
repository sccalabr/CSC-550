#ifndef SQUIRRELDBCLIENT_H
#define SQUIRRELDBCLIENT_H

#include "util.h"
#include "packets.h"

int squirrelDB_init(char *addr);
int squirrelDB_put(uint8_t *key, uint8_t *value, uint32_t data_len);
int squirrelDB_get(uint8_t *key, uint8_t *value);
int squirrelDB_update(uint8_t *key, uint8_t *value, uint32_t data_len);
int squirrelDB_delete(uint8_t *key);
int squirrelDB_slave_go_down(uint8_t node);
int squirrelDB_master_go_down();
int squirrelDB_finalize();
void start_collecting();

#endif