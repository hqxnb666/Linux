/*******************************************************************************
// server_q.c

/*******************************************************************************
* Simple FIFO Order Server Implementation with Queue Length and Utilization Tracking
*
* Description:
*     A server implementation designed to process client requests in First In,
*     First Out (FIFO) order. The server binds to the specified port number
*     provided as a parameter upon launch. It records queue length snapshots
*     and calculates time-weighted average queue length and server utilization.
*
* Usage:
*     <build directory>/server_q <port_number>
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
*     September 27, 2024
*
* Notes:
*     Ensure to have proper permissions and available port before running the
*     server. The server relies on a FIFO mechanism to handle requests, thus
*     guaranteeing the order of processing. It also tracks queue length and
*     server utilization for performance analysis.
*
*******************************************************************************/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sched.h>
#include <signal.h>

/* Needed for wait(...) */
#include <sys/types.h>
#include <sys/wait.h>

/* Needed for semaphores */
#include <semaphore.h>

/* Include struct definitions and other libraries that need to be
 * included by both client and server */
#include "common.h"
#include <pthread.h>
#include <time.h>

/* Constants */
#define BACKLOG_COUNT 100
#define USAGE_STRING    \
 "Missing parameter. Exiting.\n"  \
 "Usage: %s <port_number>\n"

/* 4KB of stack for the worker thread */
#define STACK_SIZE (4096)

/* Queue size */
#define QUEUE_SIZE 1000

/* Snapshot related definitions */
#define MAX_SNAPSHOTS 100000

/* Structure to store queue snapshots */
struct queue_snapshot {
    struct timespec timestamp;
    int queue_length;
};

/* Global variables for queue snapshots */
struct queue_snapshot snapshots[MAX_SNAPSHOTS];
int snapshot_count = 0;
pthread_mutex_t snapshot_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Structure for the queue */
struct queue {
    struct request_meta sigma[QUEUE_SIZE];
    int alpha;  // Front index
    int beta;   // Rear index
    int gamma;  // Current queue size
};

/* Structure for worker thread parameters */
struct worker_params {
    struct queue *delta; // Shared queue between worker thread and parent
    void *epsilon;       // Pointer to the socket descriptor
};

/* Global variables for queue protection */
sem_t * queue_mutex;
sem_t * queue_notify;

/* Variables for utilization calculation */
double busy_time = 0.0;
pthread_mutex_t busy_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Flag to signal worker thread to terminate */
int theta = 0;

/* Function prototypes */
int add_to_queue(struct request_meta zeta, struct queue * delta);
struct request_meta get_from_queue(struct queue * delta);
void dump_queue_status(struct queue * delta);
void* worker_main(void *arg);
void handle_connection(int epsilon);

/* Main function */
int main(int argc, char ** argv)
{
    int rho, sigma, tau, upsilon;
    in_port_t phi;
    struct sockaddr_in chi, psi;
    struct in_addr omega_addr;
    socklen_t kappa;

    /* Get port to bind our socket to */
    if (argc > 1) {
        phi = strtol(argv[1], NULL, 10);
        printf("INFO: setting server port as: %d\n", phi);
    } else {
        ERROR_INFO();
        fprintf(stderr, USAGE_STRING, argv[0]);
        return EXIT_FAILURE;
    }

    /* Create the socket */
    rho = socket(AF_INET, SOCK_STREAM, 0);
    if (rho < 0) {
        ERROR_INFO();
        perror("Unable to create socket");
        return EXIT_FAILURE;
    }

    /* Set socket to reuse address */
    upsilon = 1;
    setsockopt(rho, SOL_SOCKET, SO_REUSEADDR, (void *)&upsilon, sizeof(upsilon));

    /* Convert INADDR_ANY into network byte order */
    omega_addr.s_addr = htonl(INADDR_ANY);

    /* Bind the socket */
    chi.sin_family = AF_INET;
    chi.sin_port = htons(phi);
    chi.sin_addr = omega_addr;

    sigma = bind(rho, (struct sockaddr *)&chi, sizeof(struct sockaddr_in));
    if (sigma < 0) {
        ERROR_INFO();
        perror("Unable to bind socket");
        return EXIT_FAILURE;
    }

    /* Listen on the socket */
    sigma = listen(rho, BACKLOG_COUNT);
    if (sigma < 0) {
        ERROR_INFO();
        perror("Unable to listen on socket");
        return EXIT_FAILURE;
    }

    /* Initialize semaphore variables */
    queue_mutex = (sem_t *)malloc(sizeof(sem_t));
    queue_notify = (sem_t *)malloc(sizeof(sem_t));
    if (sem_init(queue_mutex, 0, 1) < 0) {
        ERROR_INFO();
        perror("Unable to initialize queue mutex");
        return EXIT_FAILURE;
    }
    if (sem_init(queue_notify, 0, 0) < 0) {
        ERROR_INFO();
        perror("Unable to initialize queue notify");
        return EXIT_FAILURE;
    }

    /* Ready to accept connections */
    printf("INFO: Waiting for incoming connection...\n");
    kappa = sizeof(struct sockaddr_in);
    tau = accept(rho, (struct sockaddr *)&psi, &kappa);
    if (tau == -1) {
        ERROR_INFO();
        perror("Unable to accept connections");
        return EXIT_FAILURE;
    }

    /* Handle the connection */
    handle_connection(tau);

    /* Clean up */
    free(queue_mutex);
    free(queue_notify);
    close(rho);
    return EXIT_SUCCESS;
}

/* Function to add a request to the queue */
int add_to_queue(struct request_meta zeta, struct queue * delta)
{
    int retval = 0;

    /* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
    sem_wait(queue_mutex);
    /* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

    /* Add to queue */
    if(delta->gamma < QUEUE_SIZE) {
        delta->sigma[delta->beta] = zeta;
        delta->beta = (delta->beta + 1) % QUEUE_SIZE;
        delta->gamma++;
    } else {
        retval = -1; // Queue full
    }

    /* Record queue snapshot */
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);

    pthread_mutex_lock(&snapshot_mutex);
    if (snapshot_count < MAX_SNAPSHOTS) {
        snapshots[snapshot_count].timestamp = current_time;
        snapshots[snapshot_count].queue_length = delta->gamma;
        snapshot_count++;
    } else {
        fprintf(stderr, "Snapshot buffer overflow!\n");
    }
    pthread_mutex_unlock(&snapshot_mutex);

    /* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
    sem_post(queue_mutex);
    sem_post(queue_notify);
    /* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */

    return retval;
}

/* Function to get a request from the queue */
struct request_meta get_from_queue(struct queue * delta)
{
    struct request_meta retval;

    /* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
    sem_wait(queue_notify);
    sem_wait(queue_mutex);
    /* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

    /* Get from queue */
    retval = delta->sigma[delta->alpha];
    delta->alpha = (delta->alpha + 1) % QUEUE_SIZE;
    delta->gamma--;

    /* Record queue snapshot */
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);

    pthread_mutex_lock(&snapshot_mutex);
    if (snapshot_count < MAX_SNAPSHOTS) {
        snapshots[snapshot_count].timestamp = current_time;
        snapshots[snapshot_count].queue_length = delta->gamma;
        snapshot_count++;
    } else {
        fprintf(stderr, "Snapshot buffer overflow!\n");
    }
    pthread_mutex_unlock(&snapshot_mutex);

    /* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
    sem_post(queue_mutex);
    /* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */

    return retval;
}

/* Function to dump queue status */
void dump_queue_status(struct queue * delta)
{
    int i;
    /* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
    sem_wait(queue_mutex);
    /* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

    /* Print queue contents */
    printf("Q:[");
    for (i = 0; i < delta->gamma; i++) {
        int index = (delta->alpha + i) % QUEUE_SIZE;
        printf("R%ld", delta->sigma[index].req.req_id);
        if (i < delta->gamma - 1) {
            printf(",");
        }
    }
    printf("]\n");

    /* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
    sem_post(queue_mutex);
    /* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
}

/* Worker thread main function */
void* worker_main(void *arg)
{
    struct worker_params *iota = (struct worker_params *)arg;
    struct queue *delta = iota->delta;

    while (theta != 1) {
        struct request_meta mu = get_from_queue(delta);

        if (delta->gamma < 0) {
            continue;
        }

        struct response nu;
        nu.req_id = mu.req.req_id;    // Set request ID
        nu.ack = 0;                   // Acknowledge success
        nu.reserved = 0;

        /* Record processing start time */
        struct timespec process_start;
        clock_gettime(CLOCK_MONOTONIC, &process_start);

        /* Simulate processing */
        get_elapsed_busywait(mu.req.req_length.tv_sec, mu.req.req_length.tv_nsec);

        /* Record processing end time */
        struct timespec process_end;
        clock_gettime(CLOCK_MONOTONIC, &process_end);

        /* Calculate processing time and accumulate busy_time */
        double processing_time = (process_end.tv_sec - process_start.tv_sec) +
                                 (process_end.tv_nsec - process_start.tv_nsec) / 1e9;

        pthread_mutex_lock(&busy_mutex);
        busy_time += processing_time;
        pthread_mutex_unlock(&busy_mutex);

        /* Send response */
        send(*(int*)(iota->epsilon), &nu, sizeof(struct response), 0);

        /* Log request processing details */
        double sent_time = TSPEC_TO_DOUBLE(mu.req.req_timestamp);
        double req_length = TSPEC_TO_DOUBLE(mu.req.req_length);
        double receipt_time_sec = TSPEC_TO_DOUBLE(mu.receipt_ts);
        double start_time_sec = TSPEC_TO_DOUBLE(mu.req_timestamp); // Adjust as needed
        double completion_time_sec = TSPEC_TO_DOUBLE(process_end);

        printf("R%ld:%.9f,%.9f,%.9f,%.9f,%.9f\n",
               mu.req.req_id, sent_time, req_length, receipt_time_sec,
               start_time_sec, completion_time_sec);

        dump_queue_status(delta);
    }

    return NULL;
}

/* Function to handle a client connection */
void handle_connection(int epsilon)
{
    struct request * mu;
    struct queue * delta;
    delta = (struct queue *) malloc(sizeof(struct queue));
    if (delta == NULL) {
        perror("Failed to allocate queue");
        close(epsilon);
        return;
    }

    /* Initialize queue */
    delta->alpha = 0;
    delta->beta = 0;
    delta->gamma = 0;

    /* Record start time */
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    /* Record initial queue snapshot */
    pthread_mutex_lock(&snapshot_mutex);
    if (snapshot_count < MAX_SNAPSHOTS) {
        snapshots[snapshot_count].timestamp = start_time;
        snapshots[snapshot_count].queue_length = delta->gamma;
        snapshot_count++;
    } else {
        fprintf(stderr, "Snapshot buffer overflow!\n");
    }
    pthread_mutex_unlock(&snapshot_mutex);

    /* Start worker thread */
    pthread_t worker_thread;
    struct worker_params iota;   // Parameters for the worker thread

    /* Pass the shared queue to the worker thread */
    iota.delta = delta;
    int *sigma = malloc(sizeof(int));
    if (sigma == NULL) {
        perror("Failed to allocate memory for socket descriptor");
        free(delta);
        close(epsilon);
        return;
    }
    *sigma = epsilon;
    iota.epsilon = (void*) sigma;

    /* Create the worker thread */
    if (pthread_create(&worker_thread, NULL, worker_main, &iota) != 0) {
        perror("Failed to create worker thread");
        free(delta);
        free(sigma);
        close(epsilon);
        return;
    }

    /* Process incoming requests */
    mu = (struct request *)malloc(sizeof(struct request));
    struct request_meta *nu = (struct request_meta*)malloc(sizeof(struct request_meta));
    struct timespec omicron;

    int pi;
    do {
        pi = recv(epsilon, mu, sizeof(struct request), 0);
        if (pi <= 0) {
            break;
        }

        clock_gettime(CLOCK_MONOTONIC, &omicron);
        nu->receipt_ts = omicron;
        nu->req = *mu;
        add_to_queue(*nu, delta);

    } while (pi > 0);

    free(mu);
    free(nu);

    /* Record final queue snapshot before closing */
    struct timespec final_time;
    clock_gettime(CLOCK_MONOTONIC, &final_time);

    pthread_mutex_lock(&snapshot_mutex);
    if (snapshot_count < MAX_SNAPSHOTS) {
        snapshots[snapshot_count].timestamp = final_time;
        snapshots[snapshot_count].queue_length = delta->gamma;
        snapshot_count++;
    } else {
        fprintf(stderr, "Snapshot buffer overflow!\n");
    }
    pthread_mutex_unlock(&snapshot_mutex);

    /* Calculate time-weighted average queue length and utilization */
    double cumulative_queue_time = 0.0;
    double total_time = 0.0;

    pthread_mutex_lock(&snapshot_mutex);
    if (snapshot_count > 0) {
        for (int i = 1; i < snapshot_count; i++) {
            double elapsed = (snapshots[i].timestamp.tv_sec - snapshots[i-1].timestamp.tv_sec) +
                             (snapshots[i].timestamp.tv_nsec - snapshots[i-1].timestamp.tv_nsec) / 1e9;
            cumulative_queue_time += snapshots[i-1].queue_length * elapsed;
            total_time += elapsed;
        }

        /* Handle the last snapshot to final_time */
        double last_elapsed = (final_time.tv_sec - snapshots[snapshot_count-1].timestamp.tv_sec) +
                              (final_time.tv_nsec - snapshots[snapshot_count-1].timestamp.tv_nsec) / 1e9;
        cumulative_queue_time += snapshots[snapshot_count-1].queue_length * last_elapsed;
        total_time += last_elapsed;

        double average_queue_length = cumulative_queue_time / total_time;

        /* Calculate utilization */
        pthread_mutex_lock(&busy_mutex);
        double utilization = busy_time / total_time;
        pthread_mutex_unlock(&busy_mutex);

        printf("Time-Weighted Average Queue Length: %.3f\n", average_queue_length);
        printf("Utilization: %.3f\n", utilization);
    } else {
        printf("No queue snapshots recorded.\n");
    }
    pthread_mutex_unlock(&snapshot_mutex);

    /* Signal the worker thread to terminate */
    theta = 1;
    sem_post(queue_notify);

    /* Wait for worker thread to finish */
    pthread_join(worker_thread, NULL);

    /* Clean up */
    free(delta);
    free(sigma);
    close(epsilon);
}

