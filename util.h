#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mpi.h"
#include <sys/signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include "perf_eval.h"

#ifndef BOOLS
#define BOOLS
#define FALSE 0
#define TRUE !FALSE
#endif

#ifndef OUTCOME
#define OUTCOME
#define SUCCESS TRUE
#define FAILURE FALSE
#endif

#ifdef DEBUG
#define D(x) x
#else
#define D(x)
#endif

#ifdef PRINT
#define P(x) x
#else
#define P(x)
#endif

#ifdef TIMING
#define T(x) x
#else
#define T(x)
#endif

#endif
