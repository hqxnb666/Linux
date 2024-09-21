/*******************************************************************************
 * Multi-threaded FIFO Order Server Implementation with Global Queue
 *
 * Description:
 *     A server implementation designed to process client requests in First In,
 *     First Out (FIFO) order using multiple worker threads. The server binds to
 *     the specified port number provided as a parameter upon launch. The server
 *     maintains a global queue shared among all worker threads.
 *
 * Usage:
 *     ./server_mt <port_number>
 *
 * Parameters:
 *     port_number - The port number to bind the server to.
 *
 * Author:
 *     Your Name
 *
 *******************************************************************************/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sched.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

/* Needed for socket programming */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Include our own headers */
#include "common.h"

/* Macro wrapper for formatting timestamp */
#define FORMAT_TIMESTAMP(ts, buffer, size) \
    snprintf(buffer, size, "%.6f", TSPEC_TO_DOUBLE(ts))

/* Maximum queue size and number of worker threads */
#ifndef QUEUE_SIZE
#define QUEUE_SIZE 500
#endif

#ifndef WORKER_THREAD_COUNT
#define WORKER_THREAD_COUNT 4
#endif

/* Structure for the queue */
struct queue {
    struct request requests[QUEUE_SIZE];
    int head;
    int tail;
    int size;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

/* Global queue */
struct queue global_queue;

/* Function prototypes */
int add_to_queue(struct queue *q, struct request req);
struct request get_from_queue(struct queue *q);
void *worker_main(void *arg);
void handle_connection(int conn_socket);
void dump_queue_status(struct queue *q);

/* Initialize the queue */
void init_queue(struct queue *q) {
    q->head = 0;
    q->tail = 0;
    q->size = 0;
    if (pthread_mutex_init(&q->mutex, NULL) != 0) {
        perror("Mutex init failed");
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_init(&q->cond, NULL) != 0) {
        perror("Condition variable init failed");
        pthread_mutex_destroy(&q->mutex);
        exit(EXIT_FAILURE);
    }
}

/* Destroy the queue */
void destroy_queue(struct queue *q) {
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);
}

/* Add a request to the queue */
int add_to_queue(struct queue *q, struct request req) {
    pthread_mutex_lock(&q->mutex);
    if (q->size >= QUEUE_SIZE) {
        pthread_mutex_unlock(&q->mutex);
        return -1; // Queue full
    }
    q->requests[q->tail] = req;
    q->tail = (q->tail + 1) % QUEUE_SIZE;
    q->size++;
    pthread_cond_signal(&q->cond); // Signal worker threads
    pthread_mutex_unlock(&q->mutex);
    return 0; // Success
}

/* Get a request from the queue */
struct request get_from_queue(struct queue *q) {
    pthread_mutex_lock(&q->mutex);
    while (q->size == 0) {
        pthread_cond_wait(&q->cond, &q->mutex); // Wait for requests
    }
    struct request req = q->requests[q->head];
    q->head = (q->head + 1) % QUEUE_SIZE;
    q->size--;
    pthread_mutex_unlock(&q->mutex);
    return req;
}

/* Function to dump the current status of the queue */
void dump_queue_status(struct queue *q) {
    pthread_mutex_lock(&q->mutex);
    printf("Q:[");
    for(int i = 0; i < q->size; i++) {
        int index = (q->head + i) % QUEUE_SIZE;
        printf("R%lu", q->requests[index].req_id);
        if(i != q->size -1 ) {
            printf(",");
        }
    }
    printf("]\n");
    pthread_mutex_unlock(&q->mutex);
}

/* Worker thread's main function */
void *worker_main(void *arg) {
    struct queue *q = (struct queue *)arg;
    while (1) {
        struct request req = get_from_queue(q);
        
        /* Record start timestamp */
        struct timespec start_ts;
        clock_gettime(CLOCK_REALTIME, &start_ts);
        
        /* Busywait for the requested time */
        double busywait_time = TSPEC_TO_DOUBLE(req.req_length);
        struct timespec busywait_spec;
        busywait_spec.tv_sec = (time_t)busywait_time;
        busywait_spec.tv_nsec = (long)((busywait_time - (time_t)busywait_time) * 1e9);
        busywait_timespec(busywait_spec);
        
        /* Record completion timestamp */
        struct timespec completion_ts;
        clock_gettime(CLOCK_REALTIME, &completion_ts);
        
        /* Prepare response */
        struct response resp;
        resp.req_id = req.req_id;
        resp.reserved = 0;
        resp.ack = 1;
        
        /* Send response */
        if (send(req.conn_socket, &resp, sizeof(resp), 0) == -1) {
            perror("send failed");
            // Continue processing next requests
        }
        
        /* Print the report */
        char sent_ts_str[32], receipt_ts_str[32], start_ts_str[32], completion_ts_str[32];
        FORMAT_TIMESTAMP(req.sent_timestamp, sent_ts_str, sizeof(sent_ts_str));
        FORMAT_TIMESTAMP(req.receipt_timestamp, receipt_ts_str, sizeof(receipt_ts_str));
        FORMAT_TIMESTAMP(start_ts, start_ts_str, sizeof(start_ts_str));
        FORMAT_TIMESTAMP(completion_ts, completion_ts_str, sizeof(completion_ts_str));
        
        printf("R%lu:%s,%.6f,%s,%s,%s\n",
               req.req_id,
               sent_ts_str,
               TSPEC_TO_DOUBLE(req.req_length),
               receipt_ts_str,
               start_ts_str,
               completion_ts_str);
        
        /* Dump queue status */
        dump_queue_status(q);
    }
    return NULL;
}

/* Function to handle client connection */
void handle_connection(int conn_socket) {
    /* Receive and enqueue requests */
    struct request req;
    ssize_t bytes_received;
    
    while (1) {
        bytes_received = recv(conn_socket, &req, sizeof(req), 0);
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                printf("Client disconnected.\n");
            } else {
                perror("recv failed");
            }
            break;
        }
        
        /* Record receipt timestamp */
        struct timespec receipt_ts;
        clock_gettime(CLOCK_REALTIME, &receipt_ts);
        req.receipt_timestamp = receipt_ts;
        req.conn_socket = conn_socket; // Set connection socket
        
        /* Enqueue the request */
        if (add_to_queue(&global_queue, req) != 0) {
            fprintf(stderr, "Queue is full. Dropping request ID: %lu\n", req.req_id);
            /* Send a failure response to the client */
            struct response resp;
            resp.req_id = req.req_id;
            resp.reserved = 0;
            resp.ack = 0; // Indicate failure
            if (send(conn_socket, &resp, sizeof(resp), 0) == -1) {
                perror("send failed");
            }
        }
    }
    
    /* Close connection socket */
    close(conn_socket);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port_number>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int port = atoi(argv[1]);
    
    /* Initialize the global queue */
    init_queue(&global_queue);
    
    /* Create worker threads */
    pthread_t workers[WORKER_THREAD_COUNT];
    for(int i = 0; i < WORKER_THREAD_COUNT; i++) {
        if (pthread_create(&workers[i], NULL, worker_main, (void *)&global_queue) != 0) {
            perror("Failed to create worker thread");
            exit(EXIT_FAILURE);
        }
    }
    
    /* Detach worker threads */
    for(int i = 0; i < WORKER_THREAD_COUNT; i++) {
        pthread_detach(workers[i]);
    }
    
    /* Create socket */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    /* Set socket options */
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    /* Bind socket */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    /* Listen */
    if (listen(sockfd, BACKLOG_COUNT) < 0) {
        perror("listen failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    printf("INFO: Server is listening on port %d\n", port);
    
    /* Accept connections */
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    while (1) {
        int conn_socket = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if (conn_socket < 0) {
            perror("accept failed");
            continue; // Continue accepting other connections
        }
        printf("INFO: Accepted connection from %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));
        
        /* Handle the connection */
        handle_connection(conn_socket);
    }
    
    /* Cleanup (unreachable in this code) */
    destroy_queue(&global_queue);
    close(sockfd);
    return 0;
}

