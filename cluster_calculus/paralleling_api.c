#include "paralleling_api.h"

static void* spin_routine(void* arg) {
	for(;;);
	return NULL;
}

int start_routines(struct routine* routines, int cpu_number, int threads_num, routine_job_t job) {
	pthread_attr_t pthread_attr;
    cpu_set_t cpu_set;

    if (pthread_attr_init(&pthread_attr) != 0)
        return -1;

    for (int i = threads_num; i < cpu_number; ++i) {
        CPU_ZERO(&cpu_set);
        CPU_SET(i, &cpu_set);

        if (pthread_attr_setaffinity_np(&pthread_attr, 
                                        sizeof(cpu_set_t), &cpu_set) != 0)
            return -1;

        if (pthread_create(&routines[i].pthread, &pthread_attr,
                            spin_routine, NULL) != 0)
            return -1;    
    }
    
    for (int i = 0; i < threads_num; ++i) {
        CPU_ZERO(&cpu_set);
        CPU_SET(i % cpu_number, &cpu_set);

        if (pthread_attr_setaffinity_np(&pthread_attr,
                                        sizeof(cpu_set_t), &cpu_set) != 0)
            return -1;

        if (pthread_create(&routines[i].pthread, &pthread_attr,
                            job, (void*)&routines[i].arg) != 0)
            return -1;    
    }


    if (pthread_attr_destroy(&pthread_attr) != 0)
        return -1;

    return 1;
}