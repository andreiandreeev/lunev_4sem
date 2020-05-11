#define _GNU_SOURCE

#include "lib.h"
#include "inet_header.h"

#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct server_routine_arg_t {
    double  left_border;
    double  right_border;
    int     tcp_sock;
    int     broadcastfd;
    double  answ;
};

struct server_routine_t {
    pthread_t               pthread;
    struct server_routine_arg_t arg;
};

void* server_routine(void*);
int distribute_tasks(struct server_routine_t*, int, int, int);
int join_tasks(struct server_routine_t*, int, double*);
int tcp_listen_sock(int, int);
int start_broadcast(int);
int accept_tcp_connection(int);

int main(int argc, char* argv[]) {
    int retval = EXIT_SUCCESS;

    if (argc != 2) {
        fprintf(stderr, "Usage: ./%s <workers num>", argv[0]);
        exit(EXIT_FAILURE);
    }

    int workers_num = parse_number(argv[1]);

    if (workers_num <= 0) {
        fprintf(stderr, "Usage: ./%s <workers num>", argv[0]);
        exit(EXIT_FAILURE);
    }

    int broadcastfd = socket(AF_INET, SOCK_DGRAM, 0);
    EXIT_ERR_IF(broadcastfd < 0, "socket");

    int turn_option_on = 1;
    GOTO_IF(setsockopt(broadcastfd, SOL_SOCKET, SO_BROADCAST,
            &turn_option_on, sizeof(turn_option_on)) < 0,
            handle_broadcastfd, "setsockopt SO_BROADCAST");

    GOTO_IF(setsockopt(broadcastfd, SOL_SOCKET, SO_REUSEADDR,
            &turn_option_on, sizeof(turn_option_on)) < 0,
            handle_broadcastfd, "setsockopt SO_REUSEADDR");

    int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    GOTO_IF(tcp_sock < 0, handle_broadcastfd, "socket");

    GOTO_IF(setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR,
            &turn_option_on, sizeof(turn_option_on)) < 0,
            handle_tcp_sock, "setsockopt SO_REUSEADDR");

    GOTO_IF(tcp_listen_sock(tcp_sock, workers_num) < 0, handle_tcp_sock,
            "tcp_listen_sock");
 

    GOTO_IF(start_broadcast(broadcastfd) < 0, handle_tcp_sock,
            "start_broadcast");

    struct server_routine_t*
    routines = (struct server_routine_t*)
                       calloc(workers_num,
                              sizeof(struct server_routine_t));
    assert(routines);

    GOTO_IF(start_broadcast(broadcastfd) < 0, handle_resource,
            "start_broadcast");


    GOTO_IF(distribute_tasks(routines, workers_num, tcp_sock, broadcastfd) < 0,
            handle_resource, "distribute_tasks");

    double answ = 0.0;
    GOTO_IF(join_tasks(routines, workers_num, &answ) < 0,
            handle_resource, "join_tasks");

    printf("Answ: %g\n", answ);

handle_resource:
    free(routines);
handle_tcp_sock:
    close(tcp_sock);
handle_broadcastfd:
    close(broadcastfd);

    return retval;
}

int tcp_listen_sock(int fd, int workers_num) {
    struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_port = htons(PORT),
      .sin_addr = htonl(INADDR_ANY)
    };

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return -1;
    }

    if (listen(fd, MAX_WORKERS_NUM) < 0) {
        perror("listen");
        return -1;
    }

    return 0;
}

int start_broadcast(int broadcastfd) {
    struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_port = htons(PORT),
      .sin_addr = htonl(INADDR_BROADCAST)
    };
    
    printf("###Broadcast###\n");
        
    int res = sendto(broadcastfd, (void*) broadcast_serv_msg,
                     sizeof(broadcast_serv_msg), 0,
                     (struct sockaddr*) &addr, sizeof(addr)); 
    if (res < 0)
    {
      perror("sendto");
      return -1;
    }

    return broadcastfd;
}

int distribute_tasks(struct server_routine_t* routines,
                      int workers_num, int tcp_sock, int broadcastfd)
{
    assert(routines);
    double cur_left = L,
           inter_len = (R - L) / workers_num;

    for (int i = 0; i < workers_num; ++i) {
        routines[i].arg.left_border   = cur_left;
        routines[i].arg.right_border  = cur_left + inter_len;
        routines[i].arg.tcp_sock      = tcp_sock;
        routines[i].arg.broadcastfd   = broadcastfd;
        routines[i].arg.answ          = 0.0;

        if (pthread_create(&routines[i].pthread,
                           NULL, server_routine, &routines[i].arg) != 0)
        {
           perror("pthread_create");
           return -1;
        }

        cur_left += inter_len;
    }

    return 0;
}

void* server_routine(void* arg) {
    struct server_routine_arg_t*
           routine_arg   = (struct server_routine_arg_t*) arg;
    double left_border   = routine_arg->left_border,
           right_border  = routine_arg->right_border,
           answ          = 0;

    int tcp_sock    = routine_arg->tcp_sock,
        broadcastfd = routine_arg->broadcastfd;

    pthread_mutex_lock(&mutex);

    while(1) {
       fd_set set;
       FD_ZERO(&set);
       FD_SET(tcp_sock, &set);

       struct timeval t = {
         .tv_sec = 5,
         .tv_usec = 0
       };
       
       printf("###Waiting for broadcast answer###\n");
       int ret = select(tcp_sock+1, &set, NULL, NULL, &t);
       EXIT_ERR_IF(ret < 0, "select");
       
       if (ret == 0) {
           printf("Time for response exceed!"
                   "Starting broadcast again\n");
           EXIT_ERR_IF(start_broadcast(broadcastfd) < 0, "start_broadcast");
           continue;            
       } else
           break;
    }

    printf("###Achieved response, opening tcp connection###\n");
    int fd = 0;
    EXIT_ERR_IF((fd = accept_tcp_connection(tcp_sock)) < 0,
                "accept_tcp_connection");

    pthread_mutex_unlock(&mutex);

    double task[] = {left_border, right_border};
    printf("###Sending task###\n");
    EXIT_ERR_IF(write(fd, task, sizeof(task)) < sizeof(task),
               "write to connection");
    EXIT_ERR_IF(read(fd, &answ, sizeof(answ)) <= 0,
                "read from connection");
    printf("###Answ %g read###\n", answ);

    routine_arg->answ = answ;

    close(fd);

    return NULL;
}

int accept_tcp_connection(int tcp_sock) {
    char buf[sizeof(broadcast_worker_msg)] = {0};
    int fd = accept(tcp_sock, NULL, NULL);
    int retval = fd;

    GOTO_IF(fd < 0, return_lbl, "accept");

    int r = read(fd, buf, sizeof(buf));

    GOTO_IF(r < 0, handle_resource, "read");
    GOTO_IF(r == 0, handle_resource,
            "Lost server--worker connection");

    buf[r-1] = '\0';

    GOTO_IF(strcmp(buf, broadcast_worker_msg) != 0,
            handle_resource, "Answer msg doesn't match");

    int turn_option_on  = 1,
        cnt             = 5,
        idle            = 5,
        intvl           = 1;

    // keeping connection alive
    GOTO_IF(setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,
            &turn_option_on, sizeof(turn_option_on)) < 0,
            handle_resource, "setsockopt SO_KEEPALIVE");

    // five try at alive checking
    GOTO_IF(setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT,
            &cnt, sizeof(cnt)) < 0,
            handle_resource, "setsockopt TCP_KEEPCNT");

    // after five second checking connection
    GOTO_IF(setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE,
            &idle, sizeof(idle)) < 0,
            handle_resource, "setsockopt TCP_KEEPIDLE");

    GOTO_IF(setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL,
            &intvl, sizeof(intvl)) < 0,
            handle_resource, "setsockopt TCP_KEEPINTVL");

    return retval;

handle_resource:
    close(fd);
return_lbl:
    return retval;
}

int join_tasks(struct server_routine_t* routines, int workers_num, double* answ)
{
    for (int i = 0; i < workers_num; ++i) {
        if (pthread_join(routines[i].pthread, NULL) < 0)
            return -1;
        *answ += routines[i].arg.answ;
    }

    return 1;
}
