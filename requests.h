#ifndef REQUESTS_H
#define REQUESTS_H

#include "util.h"

#define PEND_REQ_INTERVAL_SEC 0
#define PEND_REQ_INTERVAL_USEC 100000

typedef struct _pending_request pending_request;

struct _pending_request {
    uint8_t node;
    MPI_Request *request;
    pending_request *prev;
    pending_request *next;
};

pending_request *get_head();
pending_request *get_end();
void add_pending_request(uint8_t node, MPI_Request *request);
void remove_pending_request(pending_request *pend_req);
void my_alarm(long int usec, long int sec);

#endif
