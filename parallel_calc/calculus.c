#define _GNU_SOURCE

#include <sched.h>
#include <pthread.h>
#include <sys/sysinfo.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#define ASSERT_TRUE(expr)   do {                                                                    \
                                if (!(expr)) {                                                      \
                                    perror("");                                                     \
                                    fprintf(stderr, "in %s err: %s", __PRETTY_FUNCTION__, #expr);   \
                                    exit(EXIT_FAILURE);                                             \
                                }                                                                   \
                            } while (0);

#define min(x, y) ((x) < (y) ? (x) : (y))
#define sqr(x) ((x) * (x))


typedef struct dot_t {
    int first, second;
} dot_t;


typedef struct routine_arg_t {
    const dot_t* all_dots;
    int dots_num;
    //range of points among which the search is conducted
    // [first, last)
    int first, last;
    long min_distance;
} routine_arg_t;

typedef struct routine_t {
    pthread_t pthread;
    routine_arg_t arg;
}routine_t;

long distance_sqr(dot_t, dot_t);
void* SpinRoutine(void*);
void* BruteForceSearchRoutine(void*);
int get_number(const char*);

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "bad args number");
        fprintf(stderr, "usage: %s <threads number>", argv[0]);
        return EXIT_FAILURE;
    }

    int threads_num = get_number(argv[1]), spin_threads_num = 0;
    if ((threads_num <= 0)) {
        fprintf(stderr, "usage: %s <threads number>", argv[0]);
        return EXIT_FAILURE;
    }

    threads_num = min(get_nprocs(), threads_num);
    if (threads_num < get_nprocs()/2)
        spin_threads_num = get_nprocs()/2 - threads_num;

    int dots_number = 0;
    scanf("%d", &dots_number);
    assert(dots_number >= 0);

    dot_t* dots = (dot_t*) calloc(dots_number, sizeof(dot_t));
    assert(dots);

    for (int i = 0; i < dots_number; ++i) {
        scanf("%d", &dots[i].first);
        scanf("%d", &dots[i].second);
    }

    routine_t* routines = (routine_t*) calloc(threads_num + spin_threads_num, sizeof(routine_t));
    assert(routines);

    int first = 0, last = dots_number/threads_num;
    for (int i = 0; i < threads_num; ++i) {
        routines[i].arg.all_dots = dots;
        routines[i].arg.min_distance = LONG_MAX;
        routines[i].arg.first = first;
        routines[i].arg.last = last;
        routines[i].arg.dots_num = dots_number;
        first = last;
        last += dots_number/threads_num;
        //printf("thread: first %d and last %d\n",routines[i].arg.first, routines[i].arg.last );
    }

    pthread_attr_t pthread_attr;
    cpu_set_t cpu_set;

    for (int i = 0; i < threads_num; ++i) {
        ASSERT_TRUE(pthread_attr_init(&pthread_attr) == 0 );

        CPU_ZERO(&cpu_set);
        CPU_SET(i, &cpu_set);

        ASSERT_TRUE(pthread_attr_setaffinity_np(&pthread_attr, sizeof(cpu_set_t), &cpu_set) == 0);
        ASSERT_TRUE(pthread_create(&routines[i].pthread, &pthread_attr,
                BruteForceSearchRoutine, (void*)&routines[i].arg) == 0);
        ASSERT_TRUE(pthread_attr_destroy(&pthread_attr) == 0);
    }

    for (int i = threads_num; i < spin_threads_num + threads_num; ++i) {
        ASSERT_TRUE(pthread_attr_init(&pthread_attr) == 0 );

        CPU_ZERO(&cpu_set);
        CPU_SET(i, &cpu_set);
        ASSERT_TRUE(pthread_attr_setaffinity_np(&pthread_attr, sizeof(cpu_set_t), &cpu_set) == 0);
        ASSERT_TRUE(pthread_create(&routines[i].pthread, &pthread_attr,
                                   SpinRoutine, NULL) == 0);
        ASSERT_TRUE(pthread_attr_destroy(&pthread_attr) == 0);
    }

    for (int i = 0; i < threads_num; ++i) {
        //printf("joining thread %d\n", i);
        ASSERT_TRUE(pthread_join(routines[i].pthread, NULL) == 0);
    }

    //printf("JOINED");

    long answ = routines->arg.min_distance;
    for (int i = 0; i < threads_num; ++i)
        answ = min(answ, routines[i].arg.min_distance);

    printf("%ld", answ);

    free(routines);
    free(dots);

    return 0;
}

void *SpinRoutine(void* arg) {
    //printf("spin routine");
    for(;;);
    return NULL;
}

void *BruteForceSearchRoutine(void * argument) {
    routine_arg_t* arg = (routine_arg_t*)(argument);

    /*auto& first = arg.range.first;
    auto& last = arg.range.last;
    auto& answ = arg.min_distance;*/

    //answ = std::numeric_limits<long>::max();

    arg -> min_distance = LONG_MAX;

    //printf("left: %d, right: %d\n, dots number: %d\n", arg->first, arg->last, arg->dots_num);

    for (int i = arg -> first; i < arg -> last; ++i) {
        //printf("managing dot: %d\n", i);
        for (int j = 0; j < arg -> dots_num; ++j) {
            if (i != j)
                arg -> min_distance = min(arg -> min_distance,
                        distance_sqr(arg -> all_dots[i], arg -> all_dots[j]));
        }
    }

    return argument;
}

long distance_sqr(dot_t lhs, dot_t rhs) {
    return sqr((long)(rhs.first - lhs.first))
           + sqr((long)(rhs.second - lhs.second));
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
