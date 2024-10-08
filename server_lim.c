/*******************************************************************************
* Dual-Threaded FIFO Server Implementation w/ Queue Limit
*
* Description:
*     A server implementation designed to process client requests in First In,
*     First Out (FIFO) order. The server binds to the specified port number
*     provided as a parameter upon launch. It launches a secondary thread to
*     process incoming requests and allows to specify a maximum queue size.
*
* Usage:
*     <build directory>/server -q <queue_size> <port_number>
*
* Parameters:
*     port_number - The port number to bind the server to.
*     queue_size  - The maximum number of queued requests
*
* Author:
*     Renato Mancuso
*
* Affiliation:
*     Boston University
*
* Creation Date:
*     September 29, 2023
*
* Last Update:
*     September 25, 2024
*
* Notes:
*     Ensure to have proper permissions and available port before running the
*     server. The server relies on a FIFO mechanism to handle requests, thus
*     guaranteeing the order of processing. If the queue is full at the time a
*     new request is received, the request is rejected with a negative ack.
*
*******************************************************************************/

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
#define USAGE_STRING				\
	"Missing parameter. Exiting.\n"		\
	"Usage: %s -q <queue size> <port_number>\n"


/* START - Variables needed to protect the shared queue. DO NOT TOUCH */
sem_t * queue_mutex;
sem_t * queue_notify;
/* END - Variables needed to protect the shared queue. DO NOT TOUCH */
struct queue {
    /* MY IMPLEMENTATION  */
	struct request_meta *requests;
    int front;
    int rear;
    int size;
    int capacity;
};

struct worker_params {
    /* MY IMPLEMENTATION */
    struct queue *the_queue;
    int *socket;
    int *terminate;
};

/* Helper function to perform queue initialization */
void queue_init(struct queue * the_queue, size_t queue_size)
{
	/* MY IMPLEMENTATION with recurring queue*/
    the_queue->requests = (struct request_meta *)malloc(queue_size * sizeof(struct request_meta));
    the_queue->front = 0;
    the_queue->rear = -1;
    the_queue->size = 0;
    the_queue->capacity = queue_size;
}

/* Add a new request <request> to the shared queue <the_queue> */
int add_to_queue(struct request to_add, struct queue * the_queue, struct timespec rec_timestamp)
{
	int retval = 0;
	/* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
	sem_wait(queue_mutex);
	/* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

	/* MY IMPLEMENTATION */
	/* Make sure that the queue is not full */
	if (the_queue->size >= the_queue->capacity) {
		retval = -1;
	}
	else
	{
		/* If all good, add the item in the queue */
		the_queue->rear = (the_queue->rear + 1) % the_queue->capacity;
		the_queue->requests[the_queue->rear].req = to_add;
		the_queue->requests[the_queue->rear].receipt_ts= rec_timestamp;
		the_queue->size++;
		/* QUEUE SIGNALING FOR CONSUMER --- DO NOT TOUCH */
		sem_post(queue_notify);
	}

	/* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
	sem_post(queue_mutex);
	/* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
	return retval;
}

/* Add a new request <request> to the shared queue <the_queue> */
struct request_meta get_from_queue(struct queue * the_queue)
{
	struct request_meta retval;
	/* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
	sem_wait(queue_notify);
	sem_wait(queue_mutex);
	/* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

	/* MY IMPLEMENTATION */
	if (the_queue->size == 0) {
		retval.req.req_id = -1;
		return retval;
	}
	retval = the_queue->requests[the_queue->front];
	the_queue->front = (the_queue->front + 1) % the_queue->capacity;
	the_queue->size--;

	/* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
	sem_post(queue_mutex);
	/* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
	return retval;
}

/* Implement this method to correctly dump the status of the queue
 * following the format Q:[R<request ID>,R<request ID>,...] */
void dump_queue_status(struct queue * the_queue)
{
	int i;
	/* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
	sem_wait(queue_mutex);
	/* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

	/* MY IMPLEMENTATION */
	printf("Q:[");
	for (i = 0; i < the_queue->size; i++) 
	{
		int index = (the_queue->front + i) % the_queue->capacity;
		printf("R%ld", the_queue->requests[index].req.req_id);
		if (i < the_queue->size - 1)
		{
			printf(",");
		}
	}
	printf("]\n");
	/* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
	sem_post(queue_mutex);
	/* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
}


/* Main logic of the worker thread */
void* worker_main(void *arg)
{
    struct worker_params *iota = (struct worker_params *)arg;
    struct queue *the_queue = iota->the_queue;
    int *terminate = iota->terminate;
	struct timespec start_timestamp, completion_timestamp;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
	printf("[#WORKER#] %lf Worker Thread Alive!\n", TSPEC_TO_DOUBLE(now));
    
    while (!*terminate) {
        struct request_meta req = get_from_queue(iota->the_queue);

		if(req.req.req_id == -1) {
			continue;
		}
		
        clock_gettime(CLOCK_MONOTONIC, &start_timestamp);

        struct response resp;
        resp.req_id = req.req.req_id;
        resp.ack = 0;
        resp.reserved = 0;
        get_elapsed_busywait(req.req.req_length.tv_sec, req.req.req_length.tv_nsec);
        
        clock_gettime(CLOCK_MONOTONIC, &completion_timestamp);
        send(*(iota->socket), &resp, sizeof(struct response), 0);
        
        double sent_time = TSPEC_TO_DOUBLE(req.req.req_timestamp);
        double req_length = TSPEC_TO_DOUBLE(req.req.req_length);
        double receipt_time = TSPEC_TO_DOUBLE(req.receipt_ts);
        double start_time = TSPEC_TO_DOUBLE(start_timestamp);
        double completion_time = TSPEC_TO_DOUBLE(completion_timestamp);

        printf("R%ld:%lf,%lf,%lf,%lf,%lf\n",
               req.req.req_id, sent_time, req_length, receipt_time,
               start_time, completion_time);        
        dump_queue_status(the_queue);
    }

    return NULL;
}

/* Main function to handle connection with the client. This function
 * takes in input conn_socket and returns only when the connection
 * with the client is interrupted. */
void handle_connection(int socket, size_t queue_size)
{
	struct request * req;
	struct queue * the_queue = malloc(sizeof(struct queue));
	size_t in_bytes = queue_size;
	if (!the_queue) {
		perror("Failed to allocate memory for queue");
		return;
	}
	
	queue_init(the_queue, queue_size);

	/* Queue ready to go here. Let's start the worker thread. */
	int terminate = 0;
	pthread_t worker_thread;
	struct worker_params params = {the_queue, &socket, &terminate};

    // Create the worker thread
    if (pthread_create(&worker_thread, NULL, worker_main, &params) != 0) {
        free(the_queue->requests);
        free(the_queue);
        close(socket);
        return;
    }

	/* We are ready to proceed with the rest of the request
	 * handling logic. */

	/* REUSE LOGIC FROM HW1 TO HANDLE THE PACKETS */

	req = (struct request *)malloc(sizeof(struct request));
	struct timespec rec_timestamp;
	do {
		in_bytes = recv(socket, req, sizeof(struct request), 0);
		clock_gettime(CLOCK_MONOTONIC, &rec_timestamp);

		/* Don't just return if in_bytes is 0 or -1. Instead
		 * skip the response and break out of the loop in an
		 * orderly fashion so that we can de-allocate the req
		 * and resp varaibles, and shutdown the socket. */
		if (in_bytes > 0)
		{
			if(add_to_queue(*req, the_queue, rec_timestamp) == -1)
			{
				struct response resp;
				resp.req_id = req->req_id;
				resp.ack = 1;
				send(*(params.socket), &resp, sizeof(struct response), 0);
				struct timespec rej_timestamp;
				clock_gettime(CLOCK_MONOTONIC, &rej_timestamp);

				double sent_time = TSPEC_TO_DOUBLE(req->req_timestamp);
				double req_length = TSPEC_TO_DOUBLE(req->req_length);
				double rej_time = TSPEC_TO_DOUBLE(rej_timestamp);
				
				printf("X%ld:%lf,%lf,%lf\n", 
					req->req_id, sent_time, req_length, rej_time);
				
				dump_queue_status(the_queue);
			}
		}
	} while (in_bytes > 0);

	/* PERFORM ORDERLY DEALLOCATION AND OUTRO HERE */
	
	/* Ask the worker thead to terminate */
	/* ASSERT TERMINATION FLAG FOR THE WORKER THREAD */
	terminate = 1;
	/* Make sure to wake-up any thread left stuck waiting for items in the queue. DO NOT TOUCH */
	sem_post(queue_notify);

	/* Wait for orderly termination of the worker thread */	
	pthread_join(worker_thread, NULL);

	free(req);
    free(the_queue->requests);
    free(the_queue);
    
    close(socket);
}


/* Template implementation of the main function for the FIFO
 * server. The server must accept in input a command line parameter
 * with the <port number> to bind the server to. */
int main (int argc, char ** argv) {
	int sockfd, retval, accepted, optval;
	in_port_t socket_port;
	struct sockaddr_in addr, client;
	struct in_addr any_address;
	socklen_t client_len;
	size_t queue_size;
	int opt;
	/* Parse all the command line arguments */
	/* PARSE THE COMMANDS LINE: */
	/* 1. Detect the -q parameter and set aside the queue size  */
	opt = getopt(argc, argv, "q:");
    if (opt == 'q') {
        queue_size = atoi(optarg);
    } else {
        perror("Unable to get -q parameter");
        return EXIT_FAILURE;
    }
	if (optind >= argc || queue_size <= 0) {
		perror("Unable to get necessary parameters");
		return EXIT_FAILURE;
	}
	/* 2. Detect the port number to bind the server socket to (see HW1 and HW2) */
	socket_port = atoi(argv[optind]);
	/* Now onward to create the right type of socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		ERROR_INFO();
		perror("Unable to create socket");
		return EXIT_FAILURE;
	}

	/* Before moving forward, set socket to reuse address */
	optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&optval, sizeof(optval));

	/* Convert INADDR_ANY into network byte order */
	any_address.s_addr = htonl(INADDR_ANY);

	/* Time to bind the socket to the right port  */
	addr.sin_family = AF_INET;
	addr.sin_port = htons(socket_port);
	addr.sin_addr = any_address;

	/* Attempt to bind the socket with the given parameters */
	retval = bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

	if (retval < 0) {
		ERROR_INFO();
		perror("Unable to bind socket");
		return EXIT_FAILURE;
	}

	/* Let us now proceed to set the server to listen on the selected port */
	retval = listen(sockfd, BACKLOG_COUNT);

	if (retval < 0) {
		ERROR_INFO();
		perror("Unable to listen on socket");
		return EXIT_FAILURE;
	}

	/* Ready to accept connections! */
	printf("INFO: Waiting for incoming connection...\n");
	client_len = sizeof(struct sockaddr_in);
	accepted = accept(sockfd, (struct sockaddr *)&client, &client_len);

	if (accepted == -1) {
		ERROR_INFO();
		perror("Unable to accept connections");
		return EXIT_FAILURE;
	}

	/* Initialize queue protection variables. DO NOT TOUCH. */
	queue_mutex = (sem_t *)malloc(sizeof(sem_t));
	queue_notify = (sem_t *)malloc(sizeof(sem_t));
	retval = sem_init(queue_mutex, 0, 1);
	if (retval < 0) {
		ERROR_INFO();
		perror("Unable to initialize queue mutex");
		return EXIT_FAILURE;
	}
	retval = sem_init(queue_notify, 0, 0);
	if (retval < 0) {
		ERROR_INFO();
		perror("Unable to initialize queue notify");
		return EXIT_FAILURE;
	}
	/* DONE - Initialize queue protection variables. DO NOT TOUCH */

	/* Ready to handle the new connection with the client. */
	handle_connection(accepted, queue_size);

	free(queue_mutex);
	free(queue_notify);

	close(sockfd);
	return EXIT_SUCCESS;

}