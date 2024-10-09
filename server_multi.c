#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sched.h>
#include <signal.h>
#include <pthread.h>

/* Needed for wait(...) */
#include <sys/types.h>
#include <sys/wait.h>

/* Needed for semaphores */
#include <semaphore.h>

/* Include struct definitions and other libraries that need to be
 * included by both client and server */
#include "common.h"

#define BACKLOG_COUNT 100
#define USAGE_STRING \
    "Missing parameter. Exiting.\n" \
    "Usage: %s -q <queue size> -w <number of threads> <port_number>\n"

/* Mutex needed to protect the threaded printf. DO NOT TOUCH */
sem_t *print_mutex;
/* Synchronized printf for multi-threaded operation */
/* USE sync_printf(..) INSTEAD OF printf(..) FOR WORKER AND PARENT THREADS */
#define sync_printf(...)         \
    do {                         \
        sem_wait(print_mutex);   \
        printf(__VA_ARGS__);     \
        sem_post(print_mutex);   \
    } while (0)

/* START - Variables needed to protect the shared queue. DO NOT TOUCH */
sem_t *queue_lock;
sem_t *queue_signal;
/* END - Variables needed to protect the shared queue. DO NOT TOUCH */

struct request_info {
    struct request request_data;
    struct timespec receive_time;    // Time when the request was received
    struct timespec process_start;   // Time when processing starts
    struct timespec process_end;     // Time when processing completes
};

struct circular_queue {
    struct request_info *request_array;
    int head;
    int tail;
    int current_size;
    int max_size;
};

struct connection_settings {
    size_t queue_capacity;
    size_t num_workers;
};

struct worker_args {
    struct circular_queue *shared_queue; // Shared queue between worker threads and parent
    int connection_fd;
    int worker_id;
    volatile int *stop_flag;
};

void initialize_queue(struct circular_queue *queue_ptr, size_t capacity)
{
    queue_ptr->request_array = (struct request_info *)malloc(capacity * sizeof(struct request_info));
    if (queue_ptr->request_array == NULL) {
        perror("Failed to allocate memory for the queue");
        exit(EXIT_FAILURE);
    }

    queue_ptr->head = 0;
    queue_ptr->tail = 0;
    queue_ptr->current_size = 0;
    queue_ptr->max_size = capacity;
}

int enqueue_request(struct request_info new_request, struct circular_queue *queue_ptr)
{
    int result = 0;
    /* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
    sem_wait(queue_lock);
    /* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

    /* Ensure the queue is not full */
    if (queue_ptr->current_size >= queue_ptr->max_size) {
        /* Queue is full */
        result = 1;
    } else {
        /* Add the item to the queue */
        queue_ptr->request_array[queue_ptr->tail] = new_request;
        queue_ptr->tail = (queue_ptr->tail + 1) % queue_ptr->max_size;
        queue_ptr->current_size++;
        /* QUEUE SIGNALING FOR CONSUMER --- DO NOT TOUCH */
        result = 0;
        sem_post(queue_signal);
    }

    /* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
    sem_post(queue_lock);
    /* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
    return result;
}

struct request_info dequeue_request(struct circular_queue *queue_ptr)
{
    struct request_info result;
    /* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
    sem_wait(queue_signal);
    sem_wait(queue_lock);
    /* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

    result = queue_ptr->request_array[queue_ptr->head];
    queue_ptr->head = (queue_ptr->head + 1) % queue_ptr->max_size;
    queue_ptr->current_size--;

    /* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
    sem_post(queue_lock);
    /* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
    return result;
}

void display_queue_status(struct circular_queue *queue_ptr)
{
    /* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
    sem_wait(queue_lock);
    /* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

    printf("Q:[");
    for (int i = 0; i < queue_ptr->current_size; i++) {
        int index = (queue_ptr->head + i) % queue_ptr->max_size;
        printf("R%ld", queue_ptr->request_array[index].request_data.req_id);
        if (i < queue_ptr->current_size - 1) {
            printf(",");
        }
    }
    printf("]\n");

    /* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
    sem_post(queue_lock);
    /* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
}

int termination_flag = 0;
/* Main logic of the worker thread */
void *worker_thread_main(void *arg)
{
    struct timespec current_time;
    struct worker_args *args = (struct worker_args *)arg;

    /* Print the first alive message. */
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    sync_printf("[#WORKER#] %lf Worker Thread Alive!\n", TSPEC_TO_DOUBLE(current_time));

    /* Main worker logic */
    while (termination_flag != 1) {
        struct request_info req_info = dequeue_request(args->shared_queue);

        if (args->shared_queue->current_size < 0) {
            continue;
        }

        /* Record start time */
        clock_gettime(CLOCK_MONOTONIC, &req_info.process_start);

        /* Process the request */
        get_elapsed_busywait(req_info.request_data.req_length.tv_sec, req_info.request_data.req_length.tv_nsec);

        /* Record completion time */
        clock_gettime(CLOCK_MONOTONIC, &req_info.process_end);

        /* Send response */
        struct response resp_msg;
        resp_msg.req_id = req_info.request_data.req_id;
        resp_msg.ack = 0;
        resp_msg.reserved = 0;
        send(args->connection_fd, &resp_msg, sizeof(struct response), 0);

        /* Print processing information */
        sync_printf("T%d R%ld:%.9f,%.9f,%.9f,%.9f,%.9f\n",
                    args->worker_id,
                    req_info.request_data.req_id,
                    TSPEC_TO_DOUBLE(req_info.request_data.req_timestamp),
                    TSPEC_TO_DOUBLE(req_info.request_data.req_length),
                    TSPEC_TO_DOUBLE(req_info.receive_time),
                    TSPEC_TO_DOUBLE(req_info.process_start),
                    TSPEC_TO_DOUBLE(req_info.process_end));

        display_queue_status(args->shared_queue);
    }

    return NULL;
}

/* This function will control all the workers (start or stop them). */
int manage_workers(int command, size_t num_workers, struct worker_args *worker_params, pthread_t *worker_threads)
{
    if (command == 0) { // Starting all the workers
        for (int i = 0; i < num_workers; i++) {
            pthread_create(&worker_threads[i], NULL, worker_thread_main, &worker_params[i]);
        }
    } else { // Stopping all the workers
        termination_flag = 1;

        /* Wake up any threads blocked on semaphores */
        for (int j = 0; j < num_workers; j++) {
            sem_post(queue_signal);
        }
        for (int k = 0; k < num_workers; k++) {
            pthread_join(worker_threads[k], NULL);
        }
    }
    return 0;
}

/* Main function to handle connection with the client. */
void handle_client_connection(int client_socket, struct connection_settings conn_settings)
{
    struct request_info *req_info = malloc(sizeof(struct request_info));
    size_t received_bytes;

    struct circular_queue *queue_ptr = malloc(sizeof(struct circular_queue));

    initialize_queue(queue_ptr, conn_settings.queue_capacity);

    /* Allocate worker parameters and threads */
    struct worker_args *worker_params = malloc(conn_settings.num_workers * sizeof(struct worker_args));

    pthread_t *worker_threads = malloc(conn_settings.num_workers * sizeof(pthread_t));

    /* Initialize worker parameters */
    for (int i = 0; i < conn_settings.num_workers; i++) {
        worker_params[i].shared_queue = queue_ptr;
        worker_params[i].connection_fd = client_socket;
        worker_params[i].worker_id = i;
    }

    /* Start worker threads */
    manage_workers(0, conn_settings.num_workers, worker_params, worker_threads);

    do {
        /* Receive the next request */
        int received_bytes = recv(client_socket, &(req_info->request_data), sizeof(struct request), 0);
        if (received_bytes <= 0) {
            break;
        }

        /* Capture the receipt timestamp */
        clock_gettime(CLOCK_MONOTONIC, &req_info->receive_time);

        /* Try to add to the queue */
        int enqueue_result = enqueue_request(*req_info, queue_ptr);
        if (enqueue_result > 0) {
            /* Queue is full, reject the request */
            struct response reject_resp;
            reject_resp.req_id = req_info->request_data.req_id;
            reject_resp.reserved = 0;
            reject_resp.ack = 1;  // Rejected

            /* Send rejection response */
            send(client_socket, &reject_resp, sizeof(struct response), 0);

            /* Get reject timestamp */
            struct timespec reject_time;
            clock_gettime(CLOCK_MONOTONIC, &reject_time);

            /* Print rejection message */
            printf("X%ld:%lf,%lf,%lf\n",
                   req_info->request_data.req_id,
                   TSPEC_TO_DOUBLE(req_info->request_data.req_timestamp),
                   TSPEC_TO_DOUBLE(req_info->request_data.req_length),
                   TSPEC_TO_DOUBLE(reject_time));

            /* Dump queue status */
            display_queue_status(queue_ptr);
        }
    } while (received_bytes > 0);

    /* Terminate worker threads */
    termination_flag = 1;
    /* Unblock all worker threads */
    for (int i = 0; i < conn_settings.num_workers; i++) {
        sem_post(queue_signal);
    }
    manage_workers(1, conn_settings.num_workers, worker_params, worker_threads);

    /* Clean up */
    free(req_info);
    free(queue_ptr->request_array);
    free(queue_ptr);
    free(worker_params);
    free(worker_threads);
    shutdown(client_socket, SHUT_RDWR);
    close(client_socket);
    sync_printf("INFO: Client disconnected.\n");
}

/* Main function for the FIFO server. */
int main(int argc, char **argv)
{
    int server_fd, ret_val, client_fd, socket_option, opt_char;
    in_port_t port_number;
    struct sockaddr_in server_addr, client_addr;
    struct in_addr any_addr;
    socklen_t client_addr_len;

    struct connection_settings conn_settings = {0};

    /* Parse all the command line arguments */
    while ((opt_char = getopt(argc, argv, "q:w:")) != -1) {
        switch (opt_char) {
            case 'q':
                conn_settings.queue_capacity = atoi(optarg);
                break;
            case 'w':
                conn_settings.num_workers = atoi(optarg);
                break;
            default:
                fprintf(stderr, USAGE_STRING, argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, USAGE_STRING, argv[0]);
        exit(EXIT_FAILURE);
    }

    if (conn_settings.queue_capacity <= 0 || conn_settings.num_workers <= 0) {
        fprintf(stderr, USAGE_STRING, argv[0]);
        exit(EXIT_FAILURE);
    }
    port_number = atoi(argv[optind]);

    /* Now onward to create the right type of socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        ERROR_INFO();
        perror("Unable to create socket");
        return EXIT_FAILURE;
    }

    /* Before moving forward, set socket to reuse address */
    socket_option = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&socket_option, sizeof(socket_option));

    /* Convert INADDR_ANY into network byte order */
    any_addr.s_addr = htonl(INADDR_ANY);

    /* Time to bind the socket to the right port */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);
    server_addr.sin_addr = any_addr;

    /* Attempt to bind the socket with the given parameters */
    ret_val = bind(server_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));

    if (ret_val < 0) {
        ERROR_INFO();
        perror("Unable to bind socket");
        return EXIT_FAILURE;
    }

    /* Let us now proceed to set the server to listen on the selected port */
    ret_val = listen(server_fd, BACKLOG_COUNT);

    if (ret_val < 0) {
        ERROR_INFO();
        perror("Unable to listen on socket");
        return EXIT_FAILURE;
    }

    /* Ready to accept connections! */
    printf("INFO: Waiting for incoming connection...\n");
    client_addr_len = sizeof(struct sockaddr_in);
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);

    if (client_fd == -1) {
        ERROR_INFO();
        perror("Unable to accept connections");
        return EXIT_FAILURE;
    }

    /* Initialize threaded printf mutex */
    print_mutex = (sem_t *)malloc(sizeof(sem_t));
    ret_val = sem_init(print_mutex, 0, 1);
    if (ret_val < 0) {
        ERROR_INFO();
        perror("Unable to initialize printf mutex");
        return EXIT_FAILURE;
    }

    /* Initialize queue protection variables. DO NOT TOUCH. */
    queue_lock = (sem_t *)malloc(sizeof(sem_t));
    queue_signal = (sem_t *)malloc(sizeof(sem_t));
    ret_val = sem_init(queue_lock, 0, 1);
    if (ret_val < 0) {
        ERROR_INFO();
        perror("Unable to initialize queue mutex");
        return EXIT_FAILURE;
    }
    ret_val = sem_init(queue_signal, 0, 0);
    if (ret_val < 0) {
        ERROR_INFO();
        perror("Unable to initialize queue notify");
        return EXIT_FAILURE;
    }
    /* DONE - Initialize queue protection variables */

    /* Ready to handle the new connection with the client. */
    handle_client_connection(client_fd, conn_settings);

    free(queue_lock);
    free(queue_signal);
    free(print_mutex);

    close(server_fd);
    return EXIT_SUCCESS;
}
