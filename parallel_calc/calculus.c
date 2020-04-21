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

#define ASSERT_TRUE(expr)   do {                                                                    \
                                if (!(expr)) {                                                      \
                                    fprintf(stderr, "in %s err: %s\n", __PRETTY_FUNCTION__, #expr); \
                                    perror("");                                                     \
                                    exit(EXIT_FAILURE);                                             \
                                }                                                                   \
                            } while (0);    
#define L 0
#define R 40160                            
#define STEP 0.0001

#define FUNC(x) cos(x)                            
#define max(x, y) ((x)>(y) ? (x) : (y))

#if defined(DEBUG)
    #define PRINTF(...) printf(__VA_ARGS__)
#else
    #define PRINTF(...)             
#endif               

typedef struct routine_arg_t {
    double left_border, right_border;
    double answ;
} routine_arg_t;

typedef struct routine_t {
    pthread_t pthread;
    routine_arg_t arg;
} routine_t;

void* SpinRoutine(void*);
void* IntegrateRoutine(void*);
void InitRoutineArgs(routine_t*, int);
int get_number(const char*);

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "bad args number\n");
        fprintf(stderr, "usage: %s <threads number>", argv[0]);
        return EXIT_FAILURE;
    }

    int threads_num = get_number(argv[1]);
    if (threads_num <= 0) {
        fprintf(stderr, "usage: %s <threads number>", argv[0]);
        return EXIT_FAILURE;
    }

    int cpu_number = get_nprocs();

    routine_t* routines = (routine_t*) calloc(max(threads_num, cpu_number), sizeof(routine_t));
    assert(routines);

    InitRoutineArgs(routines, threads_num);

    pthread_attr_t pthread_attr;
    cpu_set_t cpu_set;

    ASSERT_TRUE(pthread_attr_init(&pthread_attr) == 0);

    for (int i = 0; i < threads_num; ++i) {

        CPU_ZERO(&cpu_set);
        CPU_SET(i, &cpu_set);

        ASSERT_TRUE(pthread_attr_setaffinity_np(&pthread_attr,
            sizeof(cpu_set_t), &cpu_set) == 0);

        ASSERT_TRUE(pthread_create(&routines[i].pthread, (i>=cpu_number) ? NULL : &pthread_attr,
            IntegrateRoutine, (void*)&routines[i].arg) == 0);    
    }

    for (int i = threads_num; i < cpu_number; ++i) {
        ASSERT_TRUE(pthread_attr_init(&pthread_attr) == 0 );

        CPU_ZERO(&cpu_set);
        CPU_SET(i, &cpu_set);

        ASSERT_TRUE(pthread_attr_setaffinity_np(&pthread_attr,
            sizeof(cpu_set_t), &cpu_set) == 0);

        ASSERT_TRUE(pthread_create(&routines[i].pthread, &pthread_attr,
            SpinRoutine, NULL) == 0);    
    }

    ASSERT_TRUE(pthread_attr_destroy(&pthread_attr) == 0);

    for (int i = 0; i < threads_num; ++i) 
        ASSERT_TRUE(pthread_join(routines[i].pthread, NULL) == 0);

    double answ = 0;
    for (int i = 0; i < threads_num; ++i)
        answ += routines[i].arg.answ;

    printf("cos(x) integral at [%d, %d] %lf\n", L, R, answ);

    free(routines);

    return 0;
}

void* IntegrateRoutine(void* arg) {
    assert(arg);
    register double cur = ((routine_arg_t*) arg) -> left_border;
    register double right_border = ((routine_arg_t*) arg) -> right_border;
    register double answ = 0;

    #ifdef DEBUG
        int i = 0;
    #endif

    while(cur < right_border) {
        #ifdef DEBUG 
        ++i;
        #endif
        answ += FUNC((cur + (cur + STEP)) / 2) * STEP;
        cur += STEP;
    }    

    ((routine_arg_t*) arg) -> answ = answ;

    PRINTF("in [%lf, %lf] %d iterations\n integral %lf\n", ((routine_arg_t*) arg) -> left_border, right_border, i, answ);
    return NULL;
}

void *SpinRoutine(void* param) {
    PRINTF("spin routine\n");
    for(;;);
    return NULL;
}

void InitRoutineArgs(routine_t* routines, int threads_num) {
    assert(routines);
    double distance = (R-L)/threads_num;
    routines[0].arg.left_border = L;
    routines[threads_num-1].arg.right_border = R;

    for (int i = 0; i < threads_num-1; ++i) {
        routines[i].arg.right_border = routines[i].arg.left_border + distance;
        routines[i+1].arg.left_border = routines[i].arg.right_border;
    }

    #ifdef DEBUG 
       for (int i = 0; i < threads_num; ++i)
            printf("thread %d [%lf, %lf]\n", i, routines[i].arg.left_border, routines[i].arg.right_border);
    #endif 
}

int get_number(const char* str) {
    char* str_end;
    long number = strtol(str, &str_end, 10);

    if (errno == ERANGE && (number == LONG_MAX || number == LONG_MIN))
        return -1;
    if ((str_end == str) || (errno != 0 && number == 0))
        return -1;

    return number;
}
