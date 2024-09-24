/*******************************************************************************
* Simple FIFO Order Server Implementation
*
* Description:
*     A server implementation designed to process client requests in First In,
*     First Out (FIFO) order. The server binds to the specified port number
*     provided as a parameter upon launch.
*
* Usage:
*     <build directory>/server <port_number>
*
* Parameters:
*     port_number - The port number to bind the server to.
*
* Author:
*     Renato Mancuso
*
* Affiliation:
*     Boston University
*
* Creation Date:
*     September 10, 2023
*
* Last Changes:
*     September 16, 2024
*
* Notes:
*     Ensure to have proper permissions and available port before running the
*     server. The server relies on a FIFO mechanism to handle requests, thus
*     guaranteeing the order of processing. For debugging or more details, refer
*     to the accompanying documentation and logs.
*
*******************************************************************************/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sched.h>
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include "common.h"

#define BACKLOG_COUNT 100
#define USAGE_STRING "Missing parameter. Exiting.\nUsage: %s <port_number>\n"

/* Semaphore for queue protection */
sem_t queue_mutex;
sem_t items_available;

/* Queue structure */
struct queue {
    struct request data[QUEUE_SIZE];
    int front;
    int rear;
    int count;
};

struct queue request_queue;

void initialize_queue(struct queue *q) {
    q->front = 0;
    q->rear = -1;
    q->count = 0;
}

int add_to_queue(struct request to_add, struct queue *q) {
    if (q->count >= QUEUE_SIZE) {
        return -1; // Queue is full
    }
    q->rear = (q->rear + 1) % QUEUE_SIZE;
    q->data[q->rear] = to_add;
    q->count++;
    return 0;
}

struct request get_from_queue(struct queue *q) {
    struct request req;
    if (q->count <= 0) {
        printf("Queue underflow\n");
        req.req_id = -1; // Indicate error
        return req;
    }
    req = q->data[q->front];
    q->front = (q->front + 1) % QUEUE_SIZE;
    q->count--;
    return req;
}

void *worker_main(void *arg) {
    struct request req;
    while (1) {
        sem_wait(&items_available);
        sem_wait(&queue_mutex);
        req = get_from_queue(&request_queue);
        sem_post(&queue_mutex);

        if (req.req_id != -1) {
            // Simulate processing request
            usleep(req.req_length.tv_nsec); // Busy-wait using usleep for simplicity
            printf("Processed request %ld\n", req.req_id);
        }
    }
}

int main(int argc, char **argv) {
    int sockfd, connfd, portno;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    pthread_t worker_thread;

    if (argc < 2) {
        fprintf(stderr, USAGE_STRING, argv[0]);
        exit(EXIT_FAILURE);
    }

    portno = atoi(argv[1]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(EXIT_FAILURE);
    }

    listen(sockfd, BACKLOG_COUNT);
    clilen = sizeof(cli_addr);

    // Initialize queue and semaphores
    initialize_queue(&request_queue);
    sem_init(&queue_mutex, 0, 1);
    sem_init(&items_available, 0, 0);

    // Start worker thread
    pthread_create(&worker_thread, NULL, worker_main, NULL);

    while (1) {
        connfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (connfd < 0) {
            perror("ERROR on accept");
            continue;
        }

        // Handle connection in the main thread or spawn a new thread for each connection
        // Simplified for this example: main thread handles connections directly
        handle_connection(connfd);
        close(connfd);
    }

    close(sockfd);
    return 0;
}
