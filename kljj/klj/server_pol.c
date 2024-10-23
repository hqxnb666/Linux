#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sched.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include "common.h"

#define BACKLOG_COUNT 100
#define USAGE_MSG \
    "Missing parameter. Exiting.\n" \
    "Usage: %s -q <queue size> " \
    "-w <workers> " \
    "-p <policy: FIFO | SJN> " \
    "<port_number>\n"

sem_t *qm;
sem_t *qn;

pthread_mutex_t print_mutex;
struct request_meta {
    struct request req;
    struct timespec recv_time;
    struct timespec start_time;
    struct timespec finish_time;
};

enum queue_policy {
    FIFO_POLICY,
    SJN_POLICY
};

struct queue {
    size_t write_idx;
    size_t read_idx;
    size_t capacity;
    size_t slots;
    struct request_meta *items;
    enum queue_policy policy_type;
};

struct connection_params {
    size_t q_size;
    size_t worker_cnt;
    enum queue_policy q_policy;
};

struct worker_params {
    int socket_fd;
    int done_flag;
    struct queue *q_ptr;
    int id;
};

enum worker_command {
    CMD_START,
    CMD_STOP
};

void queue_setup(struct queue *q, size_t size, enum queue_policy policy) {
    q->read_idx = 0;
    q->write_idx = 0;
    q->capacity = size;
    q->items = (struct request_meta *)malloc(sizeof(struct request_meta) * q->capacity);
    q->slots = size;
    q->policy_type = policy;
}

int enqueue(struct request_meta item, struct queue *q)
{
    int status = 0;

    sem_wait(qm);

    if (q->slots == 0) {
        status = 1;
    } else {
        if (q->policy_type == FIFO_POLICY) {
            q->items[q->write_idx] = item;
            q->write_idx = (q->write_idx + 1) % q->capacity;
        } else if (q->policy_type == SJN_POLICY) {
            size_t i, pos;
            size_t count = (q->write_idx + q->capacity - q->read_idx) % q->capacity;

            pos = q->write_idx;
            for (i = 0; i < count; ++i) {
                size_t idx = (q->read_idx + i) % q->capacity;
                if (timespec_cmp(&item.req.req_length, &q->items[idx].req.req_length) < 0) {
                    pos = idx;
                    break;
                }
            }

            size_t current = q->write_idx;
            while (current != pos) {
                size_t prev = (current == 0) ? q->capacity - 1 : current - 1;
                q->items[current] = q->items[prev];
                current = prev;
            }

            q->items[pos] = item;
            q->write_idx = (q->write_idx + 1) % q->capacity;
        }

        q->slots--;

        sem_post(qn);
    }

    sem_post(qm);

    return status;
}

struct request_meta dequeue(struct queue *q)
{
    struct request_meta result;
    sem_wait(qn);
    sem_wait(qm);

    result = q->items[q->read_idx];
    q->read_idx = (q->read_idx + 1) % q->capacity;
    q->slots++;

    sem_post(qm);
    return result;
}

void show_queue(struct queue *q)
{
    size_t i, count;
    sem_wait(qm);

    printf("Q:[");

    for (i = q->read_idx, count = 0; count < q->capacity - q->slots;
         i = (i + 1) % q->capacity, ++count)
    {
        printf("R%ld%s", q->items[i].req.req_id,
               ((count + 1 != q->capacity - q->slots) ? "," : ""));
    }

    printf("]\n");

    sem_post(qm);
}

void *worker_thread(void *arg)
{
    struct timespec current;
    struct worker_params *wp = (struct worker_params *)arg;

    clock_gettime(CLOCK_MONOTONIC, &current);
    printf("[#WORKER#] %lf Worker Thread Alive!\n", TSPEC_TO_DOUBLE(current));

    while (!wp->done_flag) {
        struct request_meta req;
        struct response response;

        req = dequeue(wp->q_ptr);

        if (wp->done_flag) {
            break;
        }

        clock_gettime(CLOCK_MONOTONIC, &req.start_time);
        busywait_timespec(req.req.req_length);
        clock_gettime(CLOCK_MONOTONIC, &req.finish_time);

        response.req_id = req.req.req_id;
        response.ack = RESP_COMPLETED;
        send(wp->socket_fd, &response, sizeof(struct response), 0);

        printf("T%d R%ld:%lf,%lf,%lf,%lf,%lf\n",
               wp->id,
               req.req.req_id,
               TSPEC_TO_DOUBLE(req.req.req_timestamp),
               TSPEC_TO_DOUBLE(req.req.req_length),
               TSPEC_TO_DOUBLE(req.recv_time),
               TSPEC_TO_DOUBLE(req.start_time),
               TSPEC_TO_DOUBLE(req.finish_time)
        );

        show_queue(wp->q_ptr);
    }

    return NULL;
}

int manage_workers(enum worker_command cmd, size_t count, struct worker_params *common_wp)
{
    static pthread_t *threads = NULL;
    static struct worker_params **wps = NULL;
    static int *thread_ids = NULL;

    if (cmd == CMD_START) {
        size_t i;
        threads = (pthread_t *)malloc(count * sizeof(pthread_t));
        wps = (struct worker_params **)malloc(count * sizeof(struct worker_params *));
        thread_ids = (int *)malloc(count * sizeof(int));

        if (!threads || !wps || !thread_ids) {
            ERROR_INFO();
            perror("Failed to allocate thread arrays.");
            return EXIT_FAILURE;
        }

        for (i = 0; i < count; ++i) {
            thread_ids[i] = -1;

            wps[i] = (struct worker_params *)malloc(sizeof(struct worker_params));
            if (!wps[i]) {
                ERROR_INFO();
                perror("Failed to allocate worker params.");
                return EXIT_FAILURE;
            }

            wps[i]->socket_fd = common_wp->socket_fd;
            wps[i]->q_ptr = common_wp->q_ptr;
            wps[i]->done_flag = 0;
            wps[i]->id = i;
        }

        for (i = 0; i < count; ++i) {
            thread_ids[i] = pthread_create(&threads[i], NULL, worker_thread, wps[i]);

            if (thread_ids[i] < 0) {
                ERROR_INFO();
                perror("Failed to create thread.");
                return EXIT_FAILURE;
            } else {
                printf("INFO: Worker thread %ld (TID = %d) started!\n",
                       i, thread_ids[i]);
            }
        }
    }
    else if (cmd == CMD_STOP) {
        size_t i;

        if (!threads || !wps || !thread_ids) {
            return EXIT_FAILURE;
        }

        for (i = 0; i < count; ++i) {
            if (thread_ids[i] < 0) {
                continue;
            }

            wps[i]->done_flag = 1;
        }

        for (i = 0; i < count; ++i) {
            if (thread_ids[i] < 0) {
                continue;
            }

            sem_post(qn);
        }

        for (i = 0; i < count; ++i) {
            pthread_join(threads[i], NULL);
            printf("INFO: Worker thread exited.\n");
        }

        for (i = 0; i < count; ++i) {
            free(wps[i]);
        }

        free(threads);
        threads = NULL;

        free(wps);
        wps = NULL;

        free(thread_ids);
        thread_ids = NULL;
    }
    else {
        ERROR_INFO();
        perror("Invalid worker command.");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void process_connection(int sock_fd, struct connection_params params)
{
    struct request_meta *req_ptr;
    struct queue *q;
    size_t received;

    struct worker_params common_wp;
    int result;

    q = (struct queue *)malloc(sizeof(struct queue));
    queue_setup(q, params.q_size, params.q_policy);

    common_wp.socket_fd = sock_fd;
    common_wp.q_ptr = q;
    result = manage_workers(CMD_START, params.worker_cnt, &common_wp);

    if (result != EXIT_SUCCESS) {
        free(q);
        manage_workers(CMD_STOP, params.worker_cnt, NULL);
        return;
    }

    req_ptr = (struct request_meta *)malloc(sizeof(struct request_meta));

    do {
        received = recv(sock_fd, &req_ptr->req, sizeof(struct request), 0);
        clock_gettime(CLOCK_MONOTONIC, &req_ptr->recv_time);

        if (received > 0) {
            result = enqueue(*req_ptr, q);

            if (result) {
                struct response resp;
                resp.req_id = req_ptr->req.req_id;
                resp.ack = RESP_REJECTED;
                send(sock_fd, &resp, sizeof(struct response), 0);

                printf("X%ld:%lf,%lf,%lf\n", req_ptr->req.req_id,
                       TSPEC_TO_DOUBLE(req_ptr->req.req_timestamp),
                       TSPEC_TO_DOUBLE(req_ptr->req.req_length),
                       TSPEC_TO_DOUBLE(req_ptr->recv_time)
                );
            }
        }
    } while (received > 0);

    manage_workers(CMD_STOP, params.worker_cnt, NULL);

    free(req_ptr);
    shutdown(sock_fd, SHUT_RDWR);
    close(sock_fd);
    printf("INFO: Client disconnected.\n");
}

int main(int argc, char **argv) {
    int fd, status, client_fd, option, opt;
    in_port_t port;
    struct sockaddr_in server_addr, client_addr;
    struct in_addr any_addr;
    socklen_t client_size;
    struct connection_params conn_p;
    conn_p.q_size = 0;
    conn_p.worker_cnt = 1;
    conn_p.q_policy = -1;

    while((opt = getopt(argc, argv, "q:w:p:")) != -1) {
        switch (opt) {
        case 'q':
            conn_p.q_size = strtol(optarg, NULL, 10);
            printf("INFO: setting queue size = %ld\n", conn_p.q_size);
            break;
        case 'w':
            conn_p.worker_cnt = strtol(optarg, NULL, 10);
            printf("INFO: setting worker count = %ld\n", conn_p.worker_cnt);
            break;
        case 'p':
            if (strcmp(optarg, "FIFO") == 0) {
                conn_p.q_policy = FIFO_POLICY;
            } else if (strcmp(optarg, "SJN") == 0) {
                conn_p.q_policy = SJN_POLICY;
            } else {
                fprintf(stderr, "Invalid policy: %s\n", optarg);
                fprintf(stderr, USAGE_MSG, argv[0]);
                exit(EXIT_FAILURE);
            }
            printf("INFO: setting queue policy = %s\n", optarg);
            break;
        default:
            fprintf(stderr, USAGE_MSG, argv[0]);
        }
    }

    if (conn_p.q_policy == -1) {
        ERROR_INFO();
        fprintf(stderr, USAGE_MSG, argv[0]);
        return EXIT_FAILURE;
    }

    if (!conn_p.q_size) {
        ERROR_INFO();
        fprintf(stderr, USAGE_MSG, argv[0]);
        return EXIT_FAILURE;
    }

    if (optind < argc) {
        port = strtol(argv[optind], NULL, 10);
        printf("INFO: setting server port as: %d\n", port);
    } else {
        ERROR_INFO();
        fprintf(stderr, USAGE_MSG, argv[0]);
        return EXIT_FAILURE;
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0) {
        ERROR_INFO();
        perror("Unable to create socket");
        return EXIT_FAILURE;
    }

    option = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&option, sizeof(option));

    any_addr.s_addr = htonl(INADDR_ANY);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = any_addr;

    status = bind(fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));

    if (status < 0) {
        ERROR_INFO();
        perror("Unable to bind socket");
        return EXIT_FAILURE;
    }

    status = listen(fd, BACKLOG_COUNT);

    if (status < 0) {
        ERROR_INFO();
        perror("Unable to listen on socket");
        return EXIT_FAILURE;
    }

    printf("INFO: Waiting for incoming connection...\n");
    client_size = sizeof(struct sockaddr_in);
    client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_size);

    if (client_fd == -1) {
        ERROR_INFO();
        perror("Unable to accept connections");
        return EXIT_FAILURE;
    }

    qm = (sem_t *)malloc(sizeof(sem_t));
    qn = (sem_t *)malloc(sizeof(sem_t));
    status = sem_init(qm, 0, 1);
    if (status < 0) {
        ERROR_INFO();
        perror("Unable to initialize queue mutex");
        return EXIT_FAILURE;
    }
    status = sem_init(qn, 0, 0);
    if (status < 0) {
        ERROR_INFO();
        perror("Unable to initialize queue notify");
        return EXIT_FAILURE;
    }

    process_connection(client_fd, conn_p);

    free(qm);
    free(qn);

    close(fd);
    return EXIT_SUCCESS;
}
