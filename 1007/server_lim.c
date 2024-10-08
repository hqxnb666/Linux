#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <time.h>
#include "common.h"
#include "timelib.h"

#define BACKLOG_COUNT 100

// Shared queue and synchronization primitives
struct queue {
    struct request_meta *requests;
    int front;
    int rear;
    int size;
    int capacity;
};

sem_t *queue_mutex;
sem_t *queue_notify;

// Worker parameters
struct worker_params {
    struct queue *the_queue;
    int *socket;
    int thread_id;
    int *terminate;
};

// Function prototypes
void *worker_main(void *arg);
void enqueue_request(struct request_meta req, struct queue *the_queue);
struct request_meta dequeue_request(struct queue *the_queue);
void queue_init(struct queue *the_queue, size_t queue_size);
void dump_queue_status(struct queue *the_queue);

int main(int argc, char *argv[]) {
    int opt, port, num_workers, sockfd;
    size_t queue_size = 0;

    // Parse command line arguments
    while ((opt = getopt(argc, argv, "q:w:")) != -1) {
        switch (opt) {
            case 'q':
                queue_size = atoi(optarg);
                if (queue_size <= 0) {
                    fprintf(stderr, "Invalid queue size. Must be positive.\n");
                    return EXIT_FAILURE;
                }
                break;
            case 'w':
                num_workers = atoi(optarg);
                if (num_workers < 1) {
                    fprintf(stderr, "Invalid number of workers. Must be greater than 0.\n");
                    return EXIT_FAILURE;
                }
                break;
            default:
                fprintf(stderr, "Usage: %s -q <queue_size> -w <workers> <port_number>\n", argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Port number not specified.\n");
        return EXIT_FAILURE;
    }
    port = atoi(argv[optind]);

    // Initialize queue and mutexes
    struct queue the_queue;
    queue_init(&the_queue, queue_size);

    queue_mutex = (sem_t *)malloc(sizeof(sem_t));
    queue_notify = (sem_t *)malloc(sizeof(sem_t));
    sem_init(queue_mutex, 0, 1);
    sem_init(queue_notify, 0, 0);

    // Create socket and bind to port
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Unable to create socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Unable to bind socket");
        return EXIT_FAILURE;
    }

    if (listen(sockfd, BACKLOG_COUNT) < 0) {
        perror("Unable to listen on socket");
        return EXIT_FAILURE;
    }

    // Launch worker threads
    pthread_t *worker_threads = malloc(num_workers * sizeof(pthread_t));
    int terminate = 0;
    struct worker_params *params = malloc(num_workers * sizeof(struct worker_params));
     {
       int i = 0;
    for ( i = 0; i < num_workers; i++) {
        params[i].the_queue = &the_queue;
        params[i].socket = &sockfd;
        params[i].thread_id = i;
        params[i].terminate = &terminate;
        pthread_create(&worker_threads[i], NULL, worker_main, &params[i]);
    }
     }
    // Accept incoming connections and enqueue requests
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    struct request req;
    struct timespec rec_timestamp;

    while (1) {
        int accepted = accept(sockfd, (struct sockaddr *)&client, &client_len);
        if (accepted < 0) {
            perror("Unable to accept connection");
            break;
        }

        ssize_t in_bytes = recv(accepted, &req, sizeof(req), 0);
        clock_gettime(CLOCK_MONOTONIC, &rec_timestamp);

        if (in_bytes > 0) {
            struct request_meta new_req;
            new_req.req = req;
            new_req.receipt_ts = rec_timestamp;

            sem_wait(queue_mutex);
            if (the_queue.size >= the_queue.capacity) {
                struct response resp = {req.req_id, 0, RESP_REJECTED};
                send(accepted, &resp, sizeof(resp), 0);
                printf("X%ld:%lf,%lf,%lf\n", req.req_id, TSPEC_TO_DOUBLE(req.req_timestamp), TSPEC_TO_DOUBLE(req.req_length), TSPEC_TO_DOUBLE(rec_timestamp));
            } else {
                enqueue_request(new_req, &the_queue);
                sem_post(queue_notify);
            }
            sem_post(queue_mutex);
        }
        close(accepted);
    }

    // Terminate worker threads
    terminate = 1;
    {
      int i = 0;
    for ( i = 0; i < num_workers; i++) {
        sem_post(queue_notify);
    }
    }
    {
      int i = 0;
    for ( i = 0; i < num_workers; i++) {
        pthread_join(worker_threads[i], NULL);
    }
    }

    // Cleanup
    free(the_queue.requests);
    free(queue_mutex);
    free(queue_notify);
    free(worker_threads);
    free(params);
    close(sockfd);

    return EXIT_SUCCESS;
}

void *worker_main(void *arg) {
    struct worker_params *params = (struct worker_params *)arg;
    struct queue *the_queue = params->the_queue;
    int thread_id = params->thread_id;
    int *terminate = params->terminate;
    struct timespec start_timestamp, completion_timestamp;

    while (!*terminate) {
        sem_wait(queue_notify);
        sem_wait(queue_mutex);

        if (the_queue->size == 0) {
            sem_post(queue_mutex);
            continue;
        }

        struct request_meta req = dequeue_request(the_queue);
        sem_post(queue_mutex);

        clock_gettime(CLOCK_MONOTONIC, &start_timestamp);
        get_elapsed_busywait(req.req.req_length.tv_sec, req.req.req_length.tv_nsec);
        clock_gettime(CLOCK_MONOTONIC, &completion_timestamp);

        struct response resp = {req.req.req_id, 0, RESP_COMPLETED};
        send(*(params->socket), &resp, sizeof(resp), 0);

        printf("T%d R%ld:%lf,%lf,%lf,%lf,%lf\n", thread_id, req.req.req_id,
               TSPEC_TO_DOUBLE(req.req.req_timestamp),
               TSPEC_TO_DOUBLE(req.req.req_length),
               TSPEC_TO_DOUBLE(req.receipt_ts),
               TSPEC_TO_DOUBLE(start_timestamp),
               TSPEC_TO_DOUBLE(completion_timestamp));
        dump_queue_status(the_queue);
    }
    return NULL;
}

void queue_init(struct queue *the_queue, size_t queue_size) {
    the_queue->requests = malloc(queue_size * sizeof(struct request_meta));
    the_queue->front = 0;
    the_queue->rear = -1;
    the_queue->size = 0;
    the_queue->capacity = queue_size;
}

void enqueue_request(struct request_meta req, struct queue *the_queue) {
    the_queue->rear = (the_queue->rear + 1) % the_queue->capacity;
    the_queue->requests[the_queue->rear] = req;
    the_queue->size++;
}

struct request_meta dequeue_request(struct queue *the_queue) {
    struct request_meta req = the_queue->requests[the_queue->front];
    the_queue->front = (the_queue->front + 1) % the_queue->capacity;
    the_queue->size--;
    return req;
}

void dump_queue_status(struct queue *the_queue) {
    sem_wait(queue_mutex);
    printf("Q:[");
    {
      int i = 0;
    for ( i = 0; i < the_queue->size; i++) {
        int index = (the_queue->front + i) % the_queue->capacity;
        printf("R%ld", the_queue->requests[index].req.req_id);
        if (i < the_queue->size - 1) {
            printf(",");
        }
    }
    }
    printf("]\n");
    sem_post(queue_mutex);
}
