#ifndef PERF_EVAL_H
#define PERF_EVAL_H

#include "util.h"
#include "squirrelDBClient.h"

#ifdef PERF_EVAL

#define TIMING

//Storage
#ifdef SMALL_STORAGE
#define MEM_SIZE 5000000
#elif LARGE_STORAGE
#define MEM_SIZE 500000000
#endif

//Blocksize
#ifdef SMALL_BS
#define MEM_BLOCK_SIZE 6
#elif MEDIUM_BS
#define MEM_BLOCK_SIZE 12
#elif LARGE_BS
#define MEM_BLOCK_SIZE 20
#endif

#define MAX_DATA_TABLE_ENTRIES MEM_SIZE / (MEM_BLOCK_SIZE - 4)

//Input data
#ifdef NUMERIC
#define DATA_TYPE 1
#elif MEDIUM_LEN_STRS
#define DATA_TYPE 2
#elif LONG_LEN_STRS 
#define DATA_TYPE 3
#elif MIX
#define DATA_TYPE 4
#endif

//Flush rate
#ifdef FLUSH_OFTEN
#define NUM_HEARTBEATS_TO_FLUSH 100
#elif FLUSH_RARELY
#define NUM_HEARTBEATS_TO_FLUSH 7500
#endif

#endif
#endif
