#ifndef PACKETS_H
#define PACKETS_H

#define OP_PUT 1
#define OP_GET 2
#define OP_UPDATE 3
#define OP_DELETE 4
#define OP_SLAVE_GO_DOWN 5
#define OP_MASTER_GO_DOWN 6
#define OP_SHUTDOWN 7

typedef struct _op_packet_t {
    uint8_t op;
}__attribute__((packed)) op_packet_t;

#endif