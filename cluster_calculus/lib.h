#pragma once

#include <stdlib.h>
#include <limits.h>
#include <sys/syscall.h>
#include <errno.h>
#include <math.h>

#define L 0
#define R 80320
#define STEP 0.001
#define PORT 8080
#define MAX_WORKERS_NUM 2056

#define FUNC(x) cos(x)
#define max(x, y) (((x) > (y)) ? (x) : (y))
#define str(x) #x

#define EXIT_ERR_IF(expr, msg) do { if ((expr)) {\
                                  perror(msg);\
                                  exit(EXIT_FAILURE);\
                               } } while(0)

#define GOTO_IF(expr, lable, msg) do { if ((expr)) {\
                                    perror(msg);\
                                    retval = -1;\
                                    goto lable;\
                                  } } while(0)

static const char broadcast_serv_msg[] = "SERVER BROADCAST MSG";
static const char broadcast_worker_msg[] = "GOTCHA";

#ifdef SYS_gettid
#define gettid() syscall(SYS_gettid)
#else
#error "SYS_gettid unavailable on this system"
#endif

int parse_number(const char* str);
