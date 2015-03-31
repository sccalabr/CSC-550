#ifndef SQUIRRELDB_H
#define SQUIRRELDB_H

#include "util.h"
#include "masterStorage.h"
#include "slaveStorage.h"
#include "packets.h"
#include "requests.h"

MPI_Comm world_comm;
MPI_Group world_group;
MPI_Group groups[5];
MPI_Comm comms[5];

#ifndef NUM_NODES_
#define NUM_NODES_
int NUM_NODES = 5;
#endif

#ifndef NUM_SLAVE_NODES_
#define NUM_SLAVE_NODES_
int NUM_SLAVE_NODES = 4;
#endif

#ifndef ACTIVE_NODES_
#define ACTIVE_NODES_
int active_nodes[5] = {TRUE, TRUE, TRUE, TRUE, TRUE};
#endif

#ifndef HEARTBEAT_ALARM_
#define HEARTBEAT_ALARM_
#define HEARTBEAT_REQ_INTERVAL_MASTER_SEC 2
#define HEARTBEAT_REQ_INTERVAL_SLAVE_SEC 7
int heartbeat_alarm = FALSE;
#endif

#ifndef NUM_HEARTBEATS_TO_FLUSH
#define NUM_HEARTBEATS_TO_FLUSH 1000
#endif

const int HEARTBEAT = 0;
const int PUT = 1;
const int GET = 2;
const int UPDATE = 3;
const int DELETE = 4;
const int FLUSH = 5;
const int RESTORE = 6;
const int STOP_SENDING_RECVING = 7;
const int EXIT = 8;

#endif
