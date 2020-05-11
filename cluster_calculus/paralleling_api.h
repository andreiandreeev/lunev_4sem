#pragma once

#define _GNU_SOURCE

#include <sched.h>
#include <pthread.h>
#include <sys/sysinfo.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <math.h>

struct routine_arg {
    int fd;
};

struct routine {
	pthread_t pthread;
	struct routine_arg arg;
};

typedef void* (*routine_job_t)(void*) ;

int start_routines(struct routine* routines, int cpu_number, int threads_num, routine_job_t job);