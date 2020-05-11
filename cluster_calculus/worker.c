#define _GNU_SOURCE

#include "inet_header.h"
#include "lib.h"
#include "paralleling_api.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <sys/syscall.h>
#include <string.h>

void* worker_routine(void*);
int open_udp_sock();
int establish_tcp_connection(int);

int main(int argc, char *argv[]) {
    int retval = EXIT_SUCCESS;

    if (argc != 2) {
        fprintf(stderr, "Usage: ./%s <threads num>", argv[0]);
        exit(EXIT_FAILURE);
    }

    int threads_num = parse_number(argv[1]);

    if (threads_num <= 0) {
        fprintf(stderr, "Usage: ./%s <threads num>", argv[0]);
        exit(EXIT_FAILURE);
    }

    int broadcastfd = open_udp_sock();
    EXIT_ERR_IF(broadcastfd < 0, "open_udp_sock");

    int cpu_number = get_nprocs(),
        routines_num = max(threads_num , cpu_number);

    struct routine* routines = (struct routine*)
                                calloc(routines_num, sizeof(struct routine));
    assert(routines);

    for (int i = 0; i < routines_num; ++i)
        routines[i].arg.fd = broadcastfd;

    GOTO_IF(start_routines(routines, cpu_number, threads_num, worker_routine) < 0,
            handle_resource, "start_routines");

    for (int i = 0; i < threads_num; ++i)
         GOTO_IF(pthread_join(routines[i].pthread, NULL) < 0,
                 handle_resource, "pthread_join");

handle_resource:
    close(broadcastfd);
    free(routines);

    return retval;
}

int open_udp_sock() {
    int broadcastfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (broadcastfd < 0)
        return -1;

    struct sockaddr_in addr = {
        .sin_family         = AF_INET,
        .sin_port           = htons(PORT),
        .sin_addr.s_addr    = htonl(INADDR_ANY)
    };

    if (bind(broadcastfd, (struct sockaddr*) &addr,
             sizeof(addr)) < 0)
    {
        close(broadcastfd);
        return -1;
    }

    return broadcastfd;
}

// calculating kernel
void* worker_routine(void* arg) {
    int broadcastfd = ((struct routine_arg*) arg)->fd;
    while(1) {
        printf("Worker[%ld]:###Trying establish connection to a server###\n",
                gettid());
        int tcpfd = establish_tcp_connection(broadcastfd);
        if (tcpfd < 0) {
            printf("Wroker[%ld]:###Retrying establish connection###\n",
                    gettid());
            continue;
        }
        printf("Worker[%ld]:###Connection established###\n",
                 gettid());

        double buf[2] = {0};
        fd_set set;
        FD_ZERO(&set);
        FD_SET(tcpfd, &set);

        EXIT_ERR_IF(select(tcpfd+1, &set, NULL, NULL, NULL) <= 0, "select");

        int read_ = read(tcpfd, buf, sizeof(buf));
        if (read_ <= 0) {
            perror((read_ == 0) ? "connection is broke" : "read");
            close(tcpfd);
            break;
        }

        
        printf("Worker[%ld]: task [%g, %g] came\n", gettid(), buf[0], buf[1]);

        // calculating loop
        double answ = 0, cur = buf[0], right = buf[1];
        while(cur < right) {
            double middle = (cur + (cur + STEP))/2;
            answ += FUNC(middle) * STEP;
            cur += STEP;
        }

        printf("Worker[%ld]: task result = %g\n", gettid(), answ);
        int sent = write(tcpfd, &answ, sizeof(answ));

        EXIT_ERR_IF(sent < 0 || sent != sizeof(answ),
                    "write result to socket");

        close(tcpfd);
        break;
    }

    return NULL;
}

int establish_tcp_connection(int broadcastfd) {
    char msg_buf[sizeof(broadcast_serv_msg)] = {0};
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int retval = 0;

    int read_ = recvfrom(broadcastfd, msg_buf, sizeof(msg_buf), 0,
                        (struct sockaddr*) &addr, &len);

    GOTO_IF(read_ < 0, return_lbl, "recvfrom");

    addr.sin_port = htons(PORT);
    msg_buf[read_-1] = '\0';

    GOTO_IF(strcmp(msg_buf, broadcast_serv_msg) != 0,
            return_lbl, "Wrong message from server :(");

    int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    GOTO_IF(tcp_sock < 0, return_lbl, "socket");

    // return open socket if everything is ok
    retval = tcp_sock;

    int turn_option_on = 1;
    struct timeval t = {
        .tv_sec = 5,
        .tv_usec = 0
    };

    // keeping connection alive
    GOTO_IF(setsockopt(tcp_sock, SOL_SOCKET, SO_KEEPALIVE,
            &turn_option_on, sizeof(turn_option_on)) < 0,
            handle_resource, "setsockopt SO_KEEPALIVE");

    // one try at alive checking
    GOTO_IF(setsockopt(tcp_sock, IPPROTO_TCP, TCP_KEEPCNT,
            &turn_option_on, sizeof(turn_option_on)) < 0,
            handle_resource, "setsockopt TCP_KEEPCNT");

    // after one second checking connection
    GOTO_IF(setsockopt(tcp_sock, IPPROTO_TCP, TCP_KEEPIDLE,
            &turn_option_on, sizeof(turn_option_on)) < 0,
            handle_resource, "setsockopt TCP_KEEPIDLE");

    // set timeout
    GOTO_IF(setsockopt(tcp_sock, SOL_SOCKET, SO_RCVTIMEO,
            &t, sizeof(t)) < 0,
            handle_resource, "setsockopt SO_KEEPALIVE");

    GOTO_IF(connect(tcp_sock, (struct sockaddr*) &addr, sizeof(addr)) < 0,
            handle_resource, "connect");

    int sent = send(tcp_sock, broadcast_worker_msg, sizeof(broadcast_worker_msg), 0);
    GOTO_IF((sent < 0 || sent != sizeof(broadcast_worker_msg)),
            handle_resource, "send");

    return retval;

handle_resource:
    close(tcp_sock);
return_lbl:
    return retval;
}
