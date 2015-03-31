#include "requests.h"

pending_request *head = NULL;
pending_request *end = NULL;

pending_request *get_head() {
    return head;
}

pending_request *get_end() {
    return end;
}

void add_pending_request(uint8_t node, MPI_Request *request) {
    pending_request *new_pending_request = (pending_request *) calloc(sizeof (pending_request), 1);

    new_pending_request->node = node;
    new_pending_request->request = request;

    if (head == NULL) {
        new_pending_request->prev = NULL;
        new_pending_request->next = NULL;

        head = new_pending_request;
        end = new_pending_request;
    } else {
        new_pending_request->prev = end;
        new_pending_request->next = NULL;

        end->next = new_pending_request;
        end = new_pending_request;
    }
}

void remove_pending_request(pending_request *pend_req) {
    
    if (pend_req->prev == NULL && pend_req->next == NULL) {
        head = NULL;
        end = NULL;
    } else if (pend_req->prev == NULL && pend_req->next != NULL) {
        head = pend_req->next;
        head->prev = NULL;
    } else if (pend_req->prev != NULL && pend_req->next == NULL) {
        end = pend_req->prev;
        end->next = NULL;
    } else {
        pend_req->prev->next = pend_req->next;
        pend_req->next->prev = pend_req->prev;
    }
    
    free(pend_req->request);
    free(pend_req);
}

void my_alarm(long int usec, long int sec) {
    struct itimerval timer;
    timer.it_interval.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_value.tv_usec = usec;
    timer.it_value.tv_sec = sec;
    setitimer (ITIMER_REAL, &timer, NULL);
}