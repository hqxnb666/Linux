/*******************************************************************************
 * server_q.c
 *
 * Simple FIFO Order Server Implementation with Queue Length and Utilization Tracking
 *
 * Description:
 *     A server implementation designed to process client requests in First In,
 *     First Out (FIFO) order. The server binds to the specified port number
 *     provided as a parameter upon launch. It records queue length snapshots
 *     and calculates time-weighted average queue length and server utilization.
 *
 * Usage:
 *     ./server_q <port_number>
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
#include <string.h>
#include <unistd.h>

/* Constants */
#define BACKLOG_COUNT 100
#define USAGE_STRING    \
 "Missing parameter. Exiting.\n"  \
 "Usage: %s <port_number>\n"

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
    int epsilon;         // Socket descriptor
};

/* Global variables for queue protection */
sem_t queue_mutex;
sem_t queue_notify;

/* Variables for utilization calculation */
double busy_time_total = 0.0;
pthread_mutex_t busy_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Macro to print runtime errors with file and line number */
#define ERROR_INFO()							\
    fprintf(stderr, "Runtime error at %s:%d\n", __FILE__, __LINE__)

/* Macro to convert timespec to double seconds */
#define TSPEC_TO_DOUBLE(spec) ((double)(spec.tv_sec) + (double)(spec.tv_nsec)/NANO_IN_SEC)

/* Function to add a new request to the shared queue */
int add_to_queue(struct request_meta zeta, struct queue * delta)
{
    int retval = 0;

    /* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
    sem_wait(&queue_mutex);
    /* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

    /* Write to the queue */
    if(delta->gamma < QUEUE_SIZE) {
        delta->sigma[delta->beta] = zeta;
        delta->beta = (delta->beta + 1) % QUEUE_SIZE;
        delta->gamma++;
    } else {
        retval = -1;
    }

    /* Record queue state change */
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
    sem_post(&queue_mutex);
    sem_post(&queue_notify);
    /* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
    return retval;
}

/* Function to retrieve a request from the shared queue */
struct request_meta get_from_queue(struct queue * delta)
{
    struct request_meta retval;

    /* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
    sem_wait(&queue_notify);
    sem_wait(&queue_mutex);
    /* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

    /* Read from the queue */
    retval = delta->sigma[delta->alpha];
    delta->alpha = (delta->alpha + 1) % QUEUE_SIZE;
    delta->gamma--;

    /* Record queue state change */
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
    sem_post(&queue_mutex);
    /* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
    return retval;
}

/* Function to simulate busy-waiting (processing time) */
void get_elapsed_busywait(long sec, long nsec)
{
    struct timespec start, current;
    clock_gettime(CLOCK_MONOTONIC, &start);
    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &current);
        long elapsed_sec = current.tv_sec - start.tv_sec;
        long elapsed_nsec = current.tv_nsec - start.tv_nsec;
        if (elapsed_nsec < 0) {
            elapsed_sec--;
            elapsed_nsec += 1000000000L;
        }
        if (elapsed_sec >= sec && elapsed_nsec >= nsec) {
            break;
        }
    }
}

/* Function to dump the current queue status */
void dump_queue_status(struct queue * delta)
{
    int i;
    /* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
    sem_wait(&queue_mutex);
    /* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

    /* Write queue status to stdout */
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
    sem_post(&queue_mutex);
    /* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
}

/* Worker thread main function */
void* worker_main(void *arg)
{
    // 允许线程被取消
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    struct worker_params *params = (struct worker_params *)arg;
    struct queue *delta = params->delta;
    int sockfd = params->epsilon;

    while (1) {
        // Retrieve a request from the queue
        struct request_meta mu = get_from_queue(delta);

        // Prepare the response
        struct response nu;
        nu.req_id = mu.req.req_id;    // Set request ID
        nu.ack = 0;                   // Acknowledge success
        nu.reserved = 0;

        // Record the start processing time
        struct timespec process_start;
        clock_gettime(CLOCK_MONOTONIC, &process_start);

        // Simulate busy-waiting (processing time)
        get_elapsed_busywait(mu.req.req_length.tv_sec, mu.req.req_length.tv_nsec);

        // Record the end processing time
        struct timespec process_end;
        clock_gettime(CLOCK_MONOTONIC, &process_end);

        // Calculate processing time and accumulate to busy_time_total
        double processing_time = (process_end.tv_sec - process_start.tv_sec) +
                                 (process_end.tv_nsec - process_start.tv_nsec) / 1e9;

        pthread_mutex_lock(&busy_mutex);
        busy_time_total += processing_time;
        pthread_mutex_unlock(&busy_mutex);

        // Send the response back to the client
        send(sockfd, &nu, sizeof(struct response), 0);

        // Extract and print timing information
        double eta = TSPEC_TO_DOUBLE(mu.req.req_timestamp);
        double iota = TSPEC_TO_DOUBLE(mu.req.req_length);
        double nu_time = TSPEC_TO_DOUBLE(mu.receipt_ts);
        double lambda_time = TSPEC_TO_DOUBLE(process_start);
        double xi_time = TSPEC_TO_DOUBLE(process_end);

        printf("R%ld:%.6f,%.6f,%.6f,%.6f,%.6f\n",
               mu.req.req_id, eta, iota, nu_time, lambda_time, xi_time);
    }

    return NULL;
}

/* Function to handle a client connection */
void handle_connection(int epsilon)
{
    struct request *mu;
    struct queue *delta;
    delta = (struct queue *) malloc(sizeof(struct queue));
    if (delta == NULL) {
        perror("Failed to allocate queue");
        close(epsilon);
        return;
    }

    /* Initialize the queue */
    delta->alpha = 0;
    delta->beta = 0;
    delta->gamma = 0;

    /* Record the start time */
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    /* Record the initial queue state */
    pthread_mutex_lock(&snapshot_mutex);
    if (snapshot_count < MAX_SNAPSHOTS) {
        snapshots[snapshot_count].timestamp = start_time;
        snapshots[snapshot_count].queue_length = delta->gamma;
        snapshot_count++;
    } else {
        fprintf(stderr, "Snapshot buffer overflow!\n");
    }
    pthread_mutex_unlock(&snapshot_mutex);

    /* Start the worker thread */
    pthread_t worker_thread;
    struct worker_params params;

    params.delta = delta;
    params.epsilon = epsilon;

    if (pthread_create(&worker_thread, NULL, worker_main, &params) != 0) {
        perror("Failed to create worker thread");
        free(delta);
        close(epsilon);
        return;
    }

    /* Handle incoming requests */
    mu = (struct request *)malloc(sizeof(struct request));
    struct request_meta *nu = (struct request_meta*)malloc(sizeof(struct request_meta));
    struct timespec omicron;

    if (mu == NULL || nu == NULL) {
        perror("Failed to allocate memory for requests");
        free(mu);
        free(nu);
        pthread_cancel(worker_thread);
        pthread_join(worker_thread, NULL);
        free(delta);
        close(epsilon);
        return;
    }

    int pi;
    while((pi = recv(epsilon, mu, sizeof(struct request), 0)) > 0){
        clock_gettime(CLOCK_MONOTONIC, &omicron);
        nu->receipt_ts = omicron;
        nu->req = *mu;
        add_to_queue(*nu, delta);
    }

    free(mu);
    free(nu);

    /* Record the final queue state before closing */
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

        /* Handle the last snapshot up to the final_time */
        if (snapshot_count > 0) {
            double last_elapsed = (final_time.tv_sec - snapshots[snapshot_count-1].timestamp.tv_sec) +
                                  (final_time.tv_nsec - snapshots[snapshot_count-1].timestamp.tv_nsec) / 1e9;
            cumulative_queue_time += snapshots[snapshot_count-1].queue_length * last_elapsed;
            total_time += last_elapsed;
        }

        double average_queue_length = (total_time > 0) ? (cumulative_queue_time / total_time) : 0.0;

        /* Calculate utilization */
        double utilization = (total_time > 0) ? (busy_time_total / total_time) : 0.0;

        printf("Time-Weighted Average Queue Length: %.3f\n", average_queue_length);
        printf("Utilization: %.3f\n", utilization);
    } else {
        printf("No queue snapshots recorded.\n");
    }
    pthread_mutex_unlock(&snapshot_mutex);

    /* Terminate the worker thread */
    pthread_cancel(worker_thread);
    pthread_join(worker_thread, NULL);

    /* Clean up */
    free(delta);
    close(epsilon);
}

/* Main function */
int main(int argc, char ** argv)
{
    int rho, sigma, tau, upsilon;
    in_port_t phi;
    struct sockaddr_in chi, psi;
    struct in_addr omega;
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

    /* Now onward to create the right type of socket */
    rho = socket(AF_INET, SOCK_STREAM, 0);

    if (rho < 0) {
        ERROR_INFO();
        perror("Unable to create socket");
        return EXIT_FAILURE;
    }

    /* Before moving forward, set socket to reuse address */
    upsilon = 1;
    setsockopt(rho, SOL_SOCKET, SO_REUSEADDR, (void *)&upsilon, sizeof(upsilon));

    /* Convert INADDR_ANY into network byte order */
    omega.s_addr = htonl(INADDR_ANY);

    /* Time to bind the socket to the right port */
    chi.sin_family = AF_INET;
    chi.sin_port = htons(phi);
    chi.sin_addr = omega;

    /* Attempt to bind the socket with the given parameters */
    sigma = bind(rho, (struct sockaddr *)&chi, sizeof(struct sockaddr_in));

    if (sigma < 0) {
        ERROR_INFO();
        perror("Unable to bind socket");
        return EXIT_FAILURE;
    }

    /* Let us now proceed to set the server to listen on the selected port */
    sigma = listen(rho, BACKLOG_COUNT);

    if (sigma < 0) {
        ERROR_INFO();
        perror("Unable to listen on socket");
        return EXIT_FAILURE;
    }

    /* Initialize queue protection variables. DO NOT TOUCH. */
    sem_init(&queue_mutex, 0, 1);
    sem_init(&queue_notify, 0, 0);
    /* DONE - Initialize queue protection variables. DO NOT TOUCH */

    /* Ready to accept connections! */
    printf("INFO: Waiting for incoming connection...\n");
    kappa = sizeof(struct sockaddr_in);
    tau = accept(rho, (struct sockaddr *)&psi, &kappa);

    if (tau == -1) {
        ERROR_INFO();
        perror("Unable to accept connections");
        return EXIT_FAILURE;
    }

    /* Ready to handle the new connection with the client. */
    handle_connection(tau);

    /* Clean up semaphores */
    sem_destroy(&queue_mutex);
    sem_destroy(&queue_notify);

    close(rho);
    return EXIT_SUCCESS;
}

