/*******************************************************************************
* Single-Threaded FIFO Image Server Implementation w/ Queue Limit
*
* Description:
*     A server implementation designed to process client
*     requests for image processing in First In, First Out (FIFO)
*     order. The server binds to the specified port number provided as
*     a parameter upon launch. It launches a secondary thread to
*     process incoming requests and allows to specify a maximum queue
*     size.
*
* Usage:
*     <build directory>/server -q <queue_size> -w <workers> -p <policy> <port_number>
*
* Parameters:
*     port_number - The port number to bind the server to.
*     queue_size  - The maximum number of queued requests.
*     workers     - The number of parallel threads to process requests.
*     policy      - The queue policy to use for request dispatching.
*
* Author:
*     Renato Mancuso
*
* Affiliation:
*     Boston University
*
* Creation Date:
*     October 31, 2023
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
#include <assert.h>
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
	"Usage: %s -q <queue size> "		\
	"-w <workers: 1> "			\
	"-p <policy: FIFO> "			\
	"<port_number>\n"

/* 4KB of stack for the worker thread */
#define STACK_SIZE (4096)

/* Mutex needed to protect the threaded printf. DO NOT TOUCH */
sem_t * printf_mutex;

/* Synchronized printf for multi-threaded operation */
#define sync_printf(...)			\
	do {					\
		sem_wait(printf_mutex);		\
		printf(__VA_ARGS__);		\
		sem_post(printf_mutex);		\
	} while (0)

/* START - Variables needed to protect the shared queue. DO NOT TOUCH */
sem_t * queue_mutex;
sem_t * queue_notify;
/* END - Variables needed to protect the shared queue. DO NOT TOUCH */

/* Global array of registered images and its length -- reallocated as we go! */
struct image ** images = NULL;
uint64_t image_count = 0;

struct request_meta {
	struct request request;
	struct timespec receipt_timestamp;
	struct timespec start_timestamp;
	struct timespec completion_timestamp;
};

enum queue_policy {
	QUEUE_FIFO,
	QUEUE_SJN
};

struct queue {
	size_t wr_pos;
	size_t rd_pos;
	size_t max_size;
	size_t available;
	enum queue_policy policy;
	struct request_meta * requests;
};

enum task_issue {
    task_null,
    task_in,
    task_l1,
    task_llc
};

struct connection_params {
	size_t queue_size;
	size_t workers;
	enum queue_policy queue_policy;
	enum task_issue task_issue;
};

struct worker_params {
	int conn_socket;
	int worker_done;
	struct queue * the_queue;
	int worker_id;
	enum task_issue task_issue;
};

enum worker_command {
	WORKERS_START,
	WORKERS_STOP
};



/* Global counter for generating unique image IDs */
static uint64_t image_nnnext = 1;

/* Struct to represent each stored image entry */
struct igetry {
    uint64_t ige_try_id;
    struct image *img;
    struct igetry *next;
};

struct igetry *ish = NULL;
static pthread_mutex_t isl = PTHREAD_MUTEX_INITIALIZER;


/* Image Storage Management */
void task_added_storage(uint64_t ige_try_id, struct image *img) {
    pthread_mutex_lock(&isl);
    struct igetry *ntr = malloc(sizeof(struct igetry));
    ntr->ige_try_id = ige_try_id;
    ntr->img = img;
    ntr->next = ish;
    ish = ntr;
    pthread_mutex_unlock(&isl);
}

void remove_image_from_storage(uint64_t ige_try_id) {
    pthread_mutex_lock(&isl);
    struct igetry **dict = &ish;
    while (*dict) {
        if ((*dict)->ige_try_id == ige_try_id) {
            struct igetry *fe = *dict;
            *dict = fe->next;
            deleteImage(fe->img);
            free(fe);
            break;
        }
        dict = &(*dict)->next;
    }
    pthread_mutex_unlock(&isl);
}


void queue_init(struct queue * the_queue, size_t queue_size, enum queue_policy policy)
{
	the_queue->rd_pos = 0;
	the_queue->wr_pos = 0;
	the_queue->max_size = queue_size;
	the_queue->requests = (struct request_meta *)malloc(sizeof(struct request_meta)
						     * the_queue->max_size);
	the_queue->available = queue_size;
	the_queue->policy = policy;
}

/* Add a new request <request> to the shared queue <the_queue> */
int add_to_queue(struct request_meta to_add, struct queue * the_queue)
{
	int rst = 0;
	/* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
	sem_wait(queue_mutex);
	/* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

	/* WRITE YOUR CODE HERE! */
	/* MAKE SURE NOT TO RETURN WITHOUT GOING THROUGH THE OUTRO CODE! */

	/* Make sure that the queue is not full */
	if (the_queue->available == 0) {
		rst = 1;
	} else {
		/* If all good, add the item in the queue */
		the_queue->requests[the_queue->wr_pos] = to_add;
		the_queue->wr_pos = (the_queue->wr_pos + 1) % the_queue->max_size;
		the_queue->available--;
		/* QUEUE SIGNALING FOR CONSUMER --- DO NOT TOUCH */
		sem_post(queue_notify);
	}

	/* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
	sem_post(queue_mutex);
	/* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
	return rst;
}

/* Add a new request <request> to the shared queue <the_queue> */
struct request_meta get_from_queue(struct queue * the_queue)
{
	struct request_meta rst;
	/* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
	sem_wait(queue_notify);
	sem_wait(queue_mutex);
	/* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

	/* WRITE YOUR CODE HERE! */
	/* MAKE SURE NOT TO RETURN WITHOUT GOING THROUGH THE OUTRO CODE! */
	rst = the_queue->requests[the_queue->rd_pos];
	the_queue->rd_pos = (the_queue->rd_pos + 1) % the_queue->max_size;
	the_queue->available++;

	/* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
	sem_post(queue_mutex);
	/* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
	return rst;
}

void dump_queue_status(struct queue * the_queue)
{
	size_t i, j;
	/* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
	sem_wait(queue_mutex);
	/* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

	/* WRITE YOUR CODE HERE! */
	/* MAKE SURE NOT TO RETURN WITHOUT GOING THROUGH THE OUTRO CODE! */
	sem_wait(printf_mutex);
	printf("Q:[");

	for (i = the_queue->rd_pos, j = 0; j < the_queue->max_size - the_queue->available;
	     i = (i + 1) % the_queue->max_size, ++j)
	{
		printf("R%ld%s", the_queue->requests[i].request.req_id,
		       ((j+1 != the_queue->max_size - the_queue->available)?",":""));
	}

	printf("]\n");
	sem_post(printf_mutex);
	/* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
	sem_post(queue_mutex);
	/* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
}

void register_new_image(int conn_socket, struct request * req)
{
	/* Increase the count of registered images */
	image_count++;

	/* Reallocate array of image pointers */
	images = realloc(images, image_count * sizeof(struct image *));

	/* Read in the new image from socket */
	struct image * new_img = recvImage(conn_socket);

	/* Store its pointer at the end of the global array */
	images[image_count - 1] = new_img;

	/* Immediately provide a response to the client */
	struct response resp;
	resp.req_id = req->req_id;
	resp.img_id = image_count - 1;
	resp.ack = RESP_COMPLETED;

	send(conn_socket, &resp, sizeof(struct response), 0);
}

/* Main logic of the worker thread */
void * worker_main (void * arg)
{
    struct timespec now;
    struct worker_params * params = (struct worker_params *)arg;

    /* Print the first alive message. */
    clock_gettime(CLOCK_MONOTONIC, &now);
    sync_printf("[#WORKER#] %lf Worker Thread Alive!\n", TSPEC_TO_DOUBLE(now));

    /* Set up performance counter based on event type */
    int evt_fd = -1;
    if (params->task_issue != task_null) {
        uint64_t type, cfg;
        switch (params->task_issue) {
            case task_in:
                type = PERF_TYPE_HARDWARE;
                cfg = PERF_COUNT_HW_INSTRUCTIONS;
                break;
            case task_l1:
                type = PERF_TYPE_HW_CACHE;
                cfg = (PERF_COUNT_HW_CACHE_L1D) | (PERF_COUNT_HW_CACHE_OP_READ << 8)
                         | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
                break;
            case task_llc:
                type = PERF_TYPE_HW_CACHE;
                cfg = (PERF_COUNT_HW_CACHE_LL) | (PERF_COUNT_HW_CACHE_OP_READ << 8)
                         | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
                break;
            default:
                break;
        }
        evt_fd = setup_perf_counter(type, cfg);
        if (evt_fd == -1) {
            perror("Failed to setup perf ");
            return NULL;
        }
    }

    /* Main loop */
    while (!params->worker_done) {
        struct request_meta req;
        struct response resp;
        struct image * img = NULL;
        uint64_t ige_try_id;
        
        req = get_from_queue(params->the_queue);

        if (params->worker_done)
            break;

        clock_gettime(CLOCK_MONOTONIC, &req.start_timestamp);

        ige_try_id = req.request.img_id;
        img = images[ige_try_id];
        assert(img != NULL);

        /* Reset and enable performance counter if applicable */
        if (evt_fd != -1) {
            ioctl(evt_fd, PERF_EVENT_IOC_RESET, 0);
            ioctl(evt_fd, PERF_EVENT_IOC_ENABLE, 0);
        }

        /* Process image operation */
        switch (req.request.img_op) {
            case IMG_ROT90CLKW:
                img = rotate90Clockwise(img, NULL);
                break;
            case IMG_BLUR:
                img = blurImage(img, NULL);
                break;
            case IMG_SHARPEN:
                img = sharpenImage(img, NULL);
                break;
            case IMG_VERTEDGES:
                img = detectVerticalEdges(img, NULL);
                break;
            case IMG_HORIZEDGES:
                img = detectHorizontalEdges(img, NULL);
                break;
            default:
                break;
        }

        /* Disable the counter and read the event count if applicable */
        uint64_t ec = 0;
        if (evt_fd != -1) {
            ioctl(evt_fd, PERF_EVENT_IOC_DISABLE, 0);
            ec = read_perf_counter(evt_fd);
        }

        /* Image overwriting and ID assignment */
        if (req.request.img_op != IMG_RETRIEVE) {
            if (req.request.overwrite) {
                deleteImage(images[ige_try_id]);
                images[ige_try_id] = img;
            } else {
                ige_try_id = image_count++;
                images = realloc(images, image_count * sizeof(struct image *));
                images[ige_try_id] = img;
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &req.completion_timestamp);

        /* Prepare and send response */
        resp.req_id = req.request.req_id;
        resp.ack = RESP_COMPLETED;
        resp.img_id = ige_try_id;
        send(params->conn_socket, &resp, sizeof(struct response), 0);

        /* Send image payload if requested */
        if (req.request.img_op == IMG_RETRIEVE) {
            uint8_t err = sendImage(img, params->conn_socket);
            if (err) {
                ERROR_INFO();
                perror("Unable to send image payload to client.");
            }
        }

        /* Print the operation results and event counts */
        const char* ennmm = (params->task_issue == task_in) ? "INSTR" :
                                 (params->task_issue == task_l1) ? "L1MISS" :
                                 (params->task_issue == task_llc) ? "LLCMISS" : "";
        
        if (req.request.img_op != IMG_REGISTER) {
    		printf("T%d R%ld:%lf,%s,%d,%ld,%ld,%lf,%lf,%lf,%s,%lu\n",
           		params->worker_id, req.request.req_id,
           		TSPEC_TO_DOUBLE(req.request.req_timestamp),
           		OPCODE_TO_STRING(req.request.img_op),
           		req.request.overwrite, req.request.img_id, ige_try_id,
           		TSPEC_TO_DOUBLE(req.receipt_timestamp),
           		TSPEC_TO_DOUBLE(req.start_timestamp),
           		TSPEC_TO_DOUBLE(req.completion_timestamp),
           		ennmm, ec);
		} else {
    		// For IMG_REGISTER, omit <event name> and <event count>
    		printf("T%d R%ld:%lf,%s,%d,%ld,%ld,%lf,%lf,%lf\n",
           		params->worker_id, req.request.req_id,
           		TSPEC_TO_DOUBLE(req.request.req_timestamp),
           		OPCODE_TO_STRING(req.request.img_op),
           		req.request.overwrite, req.request.img_id, ige_try_id,
           		TSPEC_TO_DOUBLE(req.receipt_timestamp),
           		TSPEC_TO_DOUBLE(req.start_timestamp),
           		TSPEC_TO_DOUBLE(req.completion_timestamp));
		}

        dump_queue_status(params->the_queue);
    }

    /* Cleanup if evt_fd was used */
    if (evt_fd != -1) {
        close(evt_fd);
    }

    return NULL;
}

/* This function will start/stop all the worker threads wrapping
 * around the pthread_join/create() function calls */

int control_workers(enum worker_command cmd, size_t worker_count,
		    struct worker_params * common_params)
{
	/* Anything we allocate should we kept as static for easy
	 * deallocation when the STOP command is issued */
	static pthread_t * worker_pthreads = NULL;
	static struct worker_params ** worker_params = NULL;
	static int * worker_ids = NULL;


	/* Start all the workers */
	if (cmd == WORKERS_START) {
		size_t i;
		/* Allocate all structs and parameters */
		worker_pthreads = (pthread_t *)malloc(worker_count * sizeof(pthread_t));
		worker_params = (struct worker_params **)
		malloc(worker_count * sizeof(struct worker_params *));
		worker_ids = (int *)malloc(worker_count * sizeof(int));


		if (!worker_pthreads || !worker_params || !worker_ids) {
			ERROR_INFO();
			perror("Unable to allocate arrays for threads.");
			return EXIT_FAILURE;
		}


		/* Allocate and initialize as needed */
		for (i = 0; i < worker_count; ++i) {
			worker_ids[i] = -1;


			worker_params[i] = (struct worker_params *)
				malloc(sizeof(struct worker_params));


			if (!worker_params[i]) {
				ERROR_INFO();
				perror("Unable to allocate memory for thread.");
				return EXIT_FAILURE;
			}


			worker_params[i]->conn_socket = common_params->conn_socket;
			worker_params[i]->the_queue = common_params->the_queue;
			worker_params[i]->worker_done = 0;
			worker_params[i]->worker_id = i;
			worker_params[i]->task_issue = common_params->task_issue;
		}


		/* All the allocations and initialization seem okay,
		 * let's start the threads */
		for (i = 0; i < worker_count; ++i) {
			worker_ids[i] = pthread_create(&worker_pthreads[i], NULL, worker_main, worker_params[i]);


			if (worker_ids[i] < 0) {
				ERROR_INFO();
				perror("Unable to start thread.");
				return EXIT_FAILURE;
			} else {
				printf("INFO: Worker thread %ld (TID = %d) started!\n",
				       i, worker_ids[i]);
			}
		}
	}


	else if (cmd == WORKERS_STOP) {
		size_t i;


		/* Command to stop the threads issues without a start
		 * command? */
		if (!worker_pthreads || !worker_params || !worker_ids) {
			return EXIT_FAILURE;
		}


		/* First, assert all the termination flags */
		for (i = 0; i < worker_count; ++i) {
			if (worker_ids[i] < 0) {
				continue;
			}


			/* Request thread termination */
			worker_params[i]->worker_done = 1;
		}


		/* Next, unblock threads and wait for completion */
		for (i = 0; i < worker_count; ++i) {
			if (worker_ids[i] < 0) {
				continue;
			}


			sem_post(queue_notify);
		}


        for (i = 0; i < worker_count; ++i) {
            pthread_join(worker_pthreads[i],NULL);
            printf("INFO: Worker thread exited.\n");
        }


		/* Finally, do a round of deallocations */
		for (i = 0; i < worker_count; ++i) {
			free(worker_params[i]);
		}


		free(worker_pthreads);
		worker_pthreads = NULL;


		free(worker_params);
		worker_params = NULL;


		free(worker_ids);
		worker_ids = NULL;
	}


	else {
		ERROR_INFO();
		perror("Invalid thread control command.");
		return EXIT_FAILURE;
	}


	return EXIT_SUCCESS;
}

/* Main function to handle connection with the client. This function
 * takes in input conn_socket and returns only when the connection
 * with the client is interrupted. */
void handle_connection(int conn_socket, struct connection_params conn_params)
{
	struct request_meta * req;
	struct queue * the_queue;
	size_t in_bytes;

	/* The connection with the client is alive here. Let's start
	 * the worker thread. */
	struct worker_params common_worker_params;
	int res;
	struct response resp;

	/* Now handle queue allocation and initialization */
	the_queue = (struct queue *)malloc(sizeof(struct queue));
	queue_init(the_queue, conn_params.queue_size, conn_params.queue_policy);

	common_worker_params.conn_socket = conn_socket;
	common_worker_params.the_queue = the_queue;
	common_worker_params.worker_done = 0;
    common_worker_params.worker_id = 0; // For single-threaded implementation
    common_worker_params.task_issue = conn_params.task_issue; // Set event_type here

	
	res = control_workers(WORKERS_START, conn_params.workers, &common_worker_params);

	/* Do not continue if there has been a problem while starting
	 * the workers. */
	if (res != EXIT_SUCCESS) {
		free(the_queue);

		/* Stop any worker that was successfully started */
		control_workers(WORKERS_STOP, conn_params.workers, NULL);
		return;
	}

	/* We are ready to proceed with the rest of the request
	 * handling logic. */

	req = (struct request_meta *)malloc(sizeof(struct request_meta));

	do {
		in_bytes = recv(conn_socket, &req->request, sizeof(struct request), 0);
		clock_gettime(CLOCK_MONOTONIC, &req->receipt_timestamp);

		/* Don't just return if in_bytes is 0 or -1. Instead
		 * skip the response and break out of the loop in an
		 * orderly fashion so that we can de-allocate the req
		 * and resp varaibles, and shutdown the socket. */
		if (in_bytes > 0) {

			/* Handle image registration right away! */
			if(req->request.img_op == IMG_REGISTER) {
				clock_gettime(CLOCK_MONOTONIC, &req->start_timestamp);

				register_new_image(conn_socket, &req->request);

				clock_gettime(CLOCK_MONOTONIC, &req->completion_timestamp);

				sync_printf("T%ld R%ld:%lf,%s,%d,%ld,%ld,%lf,%lf,%lf\n",
				       conn_params.workers, req->request.req_id,
				       TSPEC_TO_DOUBLE(req->request.req_timestamp),
				       OPCODE_TO_STRING(req->request.img_op),
				       req->request.overwrite, req->request.img_id,
				       image_count - 1, /* Registered ID on server side */
				       TSPEC_TO_DOUBLE(req->receipt_timestamp),
				       TSPEC_TO_DOUBLE(req->start_timestamp),
				       TSPEC_TO_DOUBLE(req->completion_timestamp));

				dump_queue_status(the_queue);
				continue;
			

				/* Assign a unique img_id */
				uint64_t ige_try_id = image_nnnext++;


				// Declare `new_image` to hold the received image data
				struct image *new_image = recvImage(conn_socket);

				/* Store the image */
				task_added_storage(ige_try_id, new_image);

				/* Prepare and send the response */
				resp.req_id = req->request.req_id;
				resp.img_id = ige_try_id;
				resp.ack = RESP_COMPLETED;
				send(conn_socket, &resp, sizeof(struct response), 0);

				/* Log the operation */
				clock_gettime(CLOCK_MONOTONIC, &req->start_timestamp);
				clock_gettime(CLOCK_MONOTONIC, &req->completion_timestamp);

				sync_printf("T0 R%ld:%lf,%s,%d,%ld,%ld,%lf,%lf,%lf\n",
					req->request.req_id,
					TSPEC_TO_DOUBLE(req->request.req_timestamp),
					OPCODE_TO_STRING(req->request.img_op),
					req->request.overwrite,
					0UL,
					ige_try_id,
					TSPEC_TO_DOUBLE(req->receipt_timestamp),
					TSPEC_TO_DOUBLE(req->start_timestamp),
					TSPEC_TO_DOUBLE(req->completion_timestamp)
				);

				continue; /* Skip adding to queue */
			}	




			res = add_to_queue(*req, the_queue);

			/* The queue is full if the return value is 1 */
			if (res) {
				struct response resp;
				/* Now provide a response! */
				resp.req_id = req->request.req_id;
				resp.ack = RESP_REJECTED;
				send(conn_socket, &resp, sizeof(struct response), 0);

				sync_printf("X%ld:%lf,%lf,%lf\n", req->request.req_id,
				       TSPEC_TO_DOUBLE(req->request.req_timestamp),
				       TSPEC_TO_DOUBLE(req->request.req_length),
				       TSPEC_TO_DOUBLE(req->receipt_timestamp)
					);
			}
		}
	} while (in_bytes > 0);


	/* Stop all the worker threads. */
	control_workers(WORKERS_STOP, conn_params.workers, NULL);

	free(req);
	shutdown(conn_socket, SHUT_RDWR);
	close(conn_socket);
	printf("INFO: Client disconnected.\n");
}


/* Template implementation of the main function for the FIFO
 * server. The server must accept in input a command line parameter
 * with the <port number> to bind the server to. */
int main (int argc, char ** argv) {
	int sockfd, retval, accepted, optval, opt;
	in_port_t socket_port;
	struct sockaddr_in addr, client;
	struct in_addr any_address;
	socklen_t client_len;
	struct connection_params conn_params;
	struct worker_params common_worker_params;
	conn_params.queue_size = 0;
	conn_params.queue_policy = QUEUE_FIFO;
	conn_params.workers = 1;
	// In your main function, after parsing command-line arguments
	
	common_worker_params.task_issue = conn_params.task_issue;


	/*
	TODO: Parse -h flag for what hardware counter to profile
	*/

	/* Parse all the command line arguments */
	while((opt = getopt(argc, argv, "q:w:p:h:")) != -1) {
		switch (opt) {
		case 'q':
			conn_params.queue_size = strtol(optarg, NULL, 10);
			printf("INFO: setting queue size = %ld\n", conn_params.queue_size);
			break;
		case 'w':
			conn_params.workers = strtol(optarg, NULL, 10);
			printf("INFO: setting worker count = %ld\n", conn_params.workers);
			if (conn_params.workers != 1) {
				ERROR_INFO();
				fprintf(stderr, "Only 1 worker is supported in this implementation!\n" USAGE_STRING, argv[0]);
				return EXIT_FAILURE;
			}
			break;
		case 'p':
			if (!strcmp(optarg, "FIFO")) {
				conn_params.queue_policy = QUEUE_FIFO;
			} else {
				ERROR_INFO();
				fprintf(stderr, "Invalid queue policy.\n" USAGE_STRING, argv[0]);
				return EXIT_FAILURE;
			}
			printf("INFO: setting queue policy = %s\n", optarg);
			break;
		default: /* '?' */
			fprintf(stderr, USAGE_STRING, argv[0]);

		case 'h':

			if (!strcmp(optarg, "INSTR")) {
                conn_params.task_issue = task_in;
            } else if (!strcmp(optarg, "L1MISS")) {
                conn_params.task_issue = task_l1;
            } else if (!strcmp(optarg, "LLCMISS")) {
                conn_params.task_issue = task_llc;
            } else {
                fprintf(stderr, "Invalid event type specified with -h\n" USAGE_STRING, argv[0]);
                return EXIT_FAILURE;
            }
            printf("INFO: setting hardware event = %s\n", optarg);
            break;

		}
	}

	if (!conn_params.queue_size) {
		ERROR_INFO();
		fprintf(stderr, USAGE_STRING, argv[0]);
		return EXIT_FAILURE;
	}

	if (optind < argc) {
		socket_port = strtol(argv[optind], NULL, 10);
		printf("INFO: setting server port as: %d\n", socket_port);
	} else {
		ERROR_INFO();
		fprintf(stderr, USAGE_STRING, argv[0]);
		return EXIT_FAILURE;
	}

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

	/* Initilize threaded printf mutex */
	printf_mutex = (sem_t *)malloc(sizeof(sem_t));
	retval = sem_init(printf_mutex, 0, 1);
	if (retval < 0) {
		ERROR_INFO();
		perror("Unable to initialize printf mutex");
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
	/* DONE - Initialize queue protection variables */

	/* Ready to handle the new connection with the client. */
	handle_connection(accepted, conn_params);

	free(queue_mutex);
	free(queue_notify);

	close(sockfd);
	return EXIT_SUCCESS;
}
