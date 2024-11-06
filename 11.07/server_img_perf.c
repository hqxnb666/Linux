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

enum event_type {
    EVENT_NONE,
    EVENT_INSTR,
    EVENT_L1MISS,
    EVENT_LLCMISS
};

struct connection_params {
	size_t queue_size;
	size_t workers;
	enum queue_policy queue_policy;
	enum event_type event_type;
};

struct worker_params {
	int conn_socket;
	int worker_done;
	struct queue * the_queue;
	int worker_id;
	enum event_type event_type;
};

enum worker_command {
	WORKERS_START,
	WORKERS_STOP
};



/* Global counter for generating unique image IDs */
static uint64_t next_img_id = 1;

/* Struct to represent each stored image entry */
struct image_entry {
    uint64_t img_id;
    struct image *img;
    struct image_entry *next;
};

struct image_entry *image_storage_head = NULL;
static pthread_mutex_t image_storage_lock = PTHREAD_MUTEX_INITIALIZER;


/* Image Storage Management */
void add_image_to_storage(uint64_t img_id, struct image *img) {
    pthread_mutex_lock(&image_storage_lock);
    struct image_entry *entry_ptr = malloc(sizeof(struct image_entry));
    entry_ptr->img_id = img_id;
    entry_ptr->img = img;
    entry_ptr->next = image_storage_head;
    image_storage_head = entry_ptr;
    pthread_mutex_unlock(&image_storage_lock);
}

void remove_image_from_storage(uint64_t img_id) {
    pthread_mutex_lock(&image_storage_lock);
    struct image_entry **current = &image_storage_head;
    while (*current) {
        if ((*current)->img_id == img_id) {
            struct image_entry *entry_to_delete = *current;
            *current = entry_to_delete->next;
            deleteImage(entry_to_delete->img);
            free(entry_to_delete);
            break;
        }
        current = &(*current)->next;
    }
    pthread_mutex_unlock(&image_storage_lock);
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
	int ret_value = 0;
	/* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
	sem_wait(queue_mutex);
	/* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

	/* WRITE YOUR CODE HERE! */
	/* MAKE SURE NOT TO RETURN WITHOUT GOING THROUGH THE OUTRO CODE! */

	/* Make sure that the queue is not full */
	if (the_queue->available == 0) {
		ret_value = 1;
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
	return ret_value;
}

/* Add a new request <request> to the shared queue <the_queue> */
struct request_meta get_from_queue(struct queue * the_queue)
{
	struct request_meta ret_value;
	/* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
	sem_wait(queue_notify);
	sem_wait(queue_mutex);
	/* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

	/* WRITE YOUR CODE HERE! */
	/* MAKE SURE NOT TO RETURN WITHOUT GOING THROUGH THE OUTRO CODE! */
	ret_value = the_queue->requests[the_queue->rd_pos];
	the_queue->rd_pos = (the_queue->rd_pos + 1) % the_queue->max_size;
	the_queue->available++;

	/* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
	sem_post(queue_mutex);
	/* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
	return ret_value;
}

void dump_queue_status(struct queue * the_queue)
{
	size_t index_i, index_j;
	/* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
	sem_wait(queue_mutex);
	/* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

	/* WRITE YOUR CODE HERE! */
	/* MAKE SURE NOT TO RETURN WITHOUT GOING THROUGH THE OUTRO CODE! */
	sem_wait(printf_mutex);
	printf("Q:[");

	for (index_i = the_queue->rd_pos, index_j = 0; index_j < the_queue->max_size - the_queue->available;
	     index_i = (index_i + 1) % the_queue->max_size, ++index_j)
	{
		printf("R%ld%s", the_queue->requests[index_i].request.req_id,
		       ((index_j+1 != the_queue->max_size - the_queue->available)?",":""));
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
	struct image * image_ptr = recvImage(conn_socket);

	/* Store its pointer at the end of the global array */
	images[image_count - 1] = image_ptr;

	/* Immediately provide a response to the client */
	struct response response;
	response.req_id = req->req_id;
	response.img_id = image_count - 1;
	response.ack = RESP_COMPLETED;

	send(conn_socket, &response, sizeof(struct response), 0);
}

/* Main logic of the worker thread */
void * worker_main (void * arg)
{
    struct timespec current_time;
    struct worker_params * thread_params = (struct worker_params *)arg;

    /* Print the first alive message. */
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    sync_printf("[#WORKER#] %lf Worker Thread Alive!\n", TSPEC_TO_DOUBLE(current_time));

    /* Set up performance counter based on event type */
    int performance_counter_fd = -1;
    if (thread_params->event_type != EVENT_NONE) {
        uint64_t event_type, config;
        switch (thread_params->event_type) {
            case EVENT_INSTR:
                event_type = PERF_TYPE_HARDWARE;
                config = PERF_COUNT_HW_INSTRUCTIONS;
                break;
            case EVENT_L1MISS:
                event_type = PERF_TYPE_HW_CACHE;
                config = (PERF_COUNT_HW_CACHE_L1D) | (PERF_COUNT_HW_CACHE_OP_READ << 8)
                         | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
                break;
            case EVENT_LLCMISS:
                event_type = PERF_TYPE_HW_CACHE;
                config = (PERF_COUNT_HW_CACHE_LL) | (PERF_COUNT_HW_CACHE_OP_READ << 8)
                         | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
                break;
            default:
                break;
        }
        performance_counter_fd = setup_perf_counter(event_type, config);
        if (performance_counter_fd == -1) {
            perror("Failed to setup perf counter");
            return NULL;
        }
    }

    /* Main loop */
    while (!thread_params->worker_done) {
        struct request_meta request_data;
        struct response response_data;
        struct image * image_data = NULL;
        uint64_t image_identifier;

        request_data = get_from_queue(thread_params->the_queue);

        if (thread_params->worker_done)
            break;

        clock_gettime(CLOCK_MONOTONIC, &request_data.start_timestamp);

        image_identifier = request_data.request.img_id;
        image_data = images[image_identifier];
        assert(image_data != NULL);

        /* Reset and enable performance counter if applicable */
        if (performance_counter_fd != -1) {
            ioctl(performance_counter_fd, PERF_EVENT_IOC_RESET, 0);
            ioctl(performance_counter_fd, PERF_EVENT_IOC_ENABLE, 0);
        }

        /* Process image operation */
        switch (request_data.request.img_op) {
            case IMG_ROT90CLKW:
                image_data = rotate90Clockwise(image_data, NULL);
                break;
            case IMG_BLUR:
                image_data = blurImage(image_data, NULL);
                break;
            case IMG_SHARPEN:
                image_data = sharpenImage(image_data, NULL);
                break;
            case IMG_VERTEDGES:
                image_data = detectVerticalEdges(image_data, NULL);
                break;
            case IMG_HORIZEDGES:
                image_data = detectHorizontalEdges(image_data, NULL);
                break;
            default:
                break;
        }

        /* Disable the counter and read the event count if applicable */
        uint64_t event_count = 0;
        if (performance_counter_fd != -1) {
            ioctl(performance_counter_fd, PERF_EVENT_IOC_DISABLE, 0);
            event_count = read_perf_counter(performance_counter_fd);
        }

        /* Image overwriting and ID assignment */
        if (request_data.request.img_op != IMG_RETRIEVE) {
            if (request_data.request.overwrite) {
                deleteImage(images[image_identifier]);
                images[image_identifier] = image_data;
            } else {
                image_identifier = image_count++;
                images = realloc(images, image_count * sizeof(struct image *));
                images[image_identifier] = image_data;
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &request_data.completion_timestamp);

        /* Prepare and send response */
        response_data.req_id = request_data.request.req_id;
        response_data.ack = RESP_COMPLETED;
        response_data.img_id = image_identifier;
        send(thread_params->conn_socket, &response_data, sizeof(struct response), 0);

        /* Send image payload if requested */
        if (request_data.request.img_op == IMG_RETRIEVE) {
            uint8_t error_status = sendImage(image_data, thread_params->conn_socket);
            if (error_status) {
                ERROR_INFO();
                perror("Unable to send image payload to client.");
            }
        }

        /* Print the operation results and event counts */
        const char* event_name = (thread_params->event_type == EVENT_INSTR) ? "INSTR" :
                                 (thread_params->event_type == EVENT_L1MISS) ? "L1MISS" :
                                 (thread_params->event_type == EVENT_LLCMISS) ? "LLCMISS" : "";

        if (request_data.request.img_op != IMG_REGISTER) {
            printf("T%d R%ld:%lf,%s,%d,%ld,%ld,%lf,%lf,%lf,%s,%lu\n",
                   thread_params->worker_id, request_data.request.req_id,
                   TSPEC_TO_DOUBLE(request_data.request.req_timestamp),
                   OPCODE_TO_STRING(request_data.request.img_op),
                   request_data.request.overwrite, request_data.request.img_id, image_identifier,
                   TSPEC_TO_DOUBLE(request_data.receipt_timestamp),
                   TSPEC_TO_DOUBLE(request_data.start_timestamp),
                   TSPEC_TO_DOUBLE(request_data.completion_timestamp),
                   event_name, event_count);
        } else {
            // For IMG_REGISTER, omit <event name> and <event count>
            printf("T%d R%ld:%lf,%s,%d,%ld,%ld,%lf,%lf,%lf\n",
                   thread_params->worker_id, request_data.request.req_id,
                   TSPEC_TO_DOUBLE(request_data.request.req_timestamp),
                   OPCODE_TO_STRING(request_data.request.img_op),
                   request_data.request.overwrite, request_data.request.img_id, image_identifier,
                   TSPEC_TO_DOUBLE(request_data.receipt_timestamp),
                   TSPEC_TO_DOUBLE(request_data.start_timestamp),
                   TSPEC_TO_DOUBLE(request_data.completion_timestamp));
        }

        dump_queue_status(thread_params->the_queue);
    }

    /* Cleanup if performance_counter_fd was used */
    if (performance_counter_fd != -1) {
        close(performance_counter_fd);
    }

    return NULL;
}

/* This function will start/stop all the worker threads wrapping
 * around the pthread_join/create() function calls */

int control_workers(enum worker_command command, size_t worker_count,
                    struct worker_params * common_params)
{
    /* Anything we allocate should we kept as static for easy
     * deallocation when the STOP command is issued */
    static pthread_t * thread_handles = NULL;
    static struct worker_params ** thread_params = NULL;
    static int * thread_ids = NULL;

    /* Start all the workers */
    if (command == WORKERS_START) {
        size_t i;
        /* Allocate all structs and parameters */
        thread_handles = (pthread_t *)malloc(worker_count * sizeof(pthread_t));
        thread_params = (struct worker_params **)
        malloc(worker_count * sizeof(struct worker_params *));
        thread_ids = (int *)malloc(worker_count * sizeof(int));

        if (!thread_handles || !thread_params || !thread_ids) {
            ERROR_INFO();
            perror("Unable to allocate arrays for threads.");
            return EXIT_FAILURE;
        }

        /* Allocate and initialize as needed */
        for (i = 0; i < worker_count; ++i) {
            thread_ids[i] = -1;

            thread_params[i] = (struct worker_params *)
                malloc(sizeof(struct worker_params));

            if (!thread_params[i]) {
                ERROR_INFO();
                perror("Unable to allocate memory for thread.");
                return EXIT_FAILURE;
            }

            thread_params[i]->conn_socket = common_params->conn_socket;
            thread_params[i]->the_queue = common_params->the_queue;
            thread_params[i]->worker_done = 0;
            thread_params[i]->worker_id = i;
            thread_params[i]->event_type = common_params->event_type;
        }

        /* All the allocations and initialization seem okay,
         * let's start the threads */
        for (i = 0; i < worker_count; ++i) {
            thread_ids[i] = pthread_create(&thread_handles[i], NULL, worker_main, thread_params[i]);

            if (thread_ids[i] < 0) {
                ERROR_INFO();
                perror("Unable to start thread.");
                return EXIT_FAILURE;
            } else {
                printf("INFO: Worker thread %ld (TID = %d) started!\n",
                       i, thread_ids[i]);
            }
        }
    }
    else if (command == WORKERS_STOP) {
        size_t i;

        /* Command to stop the threads issues without a start
         * command? */
        if (!thread_handles || !thread_params || !thread_ids) {
            return EXIT_FAILURE;
        }

        /* First, assert all the termination flags */
        for (i = 0; i < worker_count; ++i) {
            if (thread_ids[i] < 0) {
                continue;
            }

            /* Request thread termination */
            thread_params[i]->worker_done = 1;
        }

        /* Next, unblock threads and wait for completion */
        for (i = 0; i < worker_count; ++i) {
            if (thread_ids[i] < 0) {
                continue;
            }

            sem_post(queue_notify);
        }

        for (i = 0; i < worker_count; ++i) {
            pthread_join(thread_handles[i],NULL);
            printf("INFO: Worker thread exited.\n");
        }

        /* Finally, do a round of deallocations */
        for (i = 0; i < worker_count; ++i) {
            free(thread_params[i]);
        }

        free(thread_handles);
        thread_handles = NULL;

        free(thread_params);
        thread_params = NULL;

        free(thread_ids);
        thread_ids = NULL;
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
void handle_client_connection(int conn_socket, struct connection_config conn_config)
{
	struct request_metadata * req_meta;
	struct queue * client_queue;
	size_t received_bytes;

	/* The connection with the client is alive here. Let's start
	 * the worker thread. */
	struct worker_config shared_worker_config;
	int result;
	struct response response_data;

	/* Now handle queue allocation and initialization */
	client_queue = (struct queue *)malloc(sizeof(struct queue));
	initialize_queue(client_queue, conn_config.queue_size, conn_config.queue_policy);

	shared_worker_config.conn_socket = conn_socket;
	shared_worker_config.client_queue = client_queue;
	shared_worker_config.worker_done = 0;
    shared_worker_config.worker_id = 0; // For single-threaded implementation
    shared_worker_config.event_type = conn_config.event_type; // Set event_type here

	
	result = control_workers(WORKERS_START, conn_config.workers, &shared_worker_config);

	/* Do not continue if there has been a problem while starting
	 * the workers. */
	if (result != EXIT_SUCCESS) {
		free(client_queue);

		/* Stop any worker that was successfully started */
		control_workers(WORKERS_STOP, conn_config.workers, NULL);
		return;
	}

	/* We are ready to proceed with the rest of the request
	 * handling logic. */

	req_meta = (struct request_metadata *)malloc(sizeof(struct request_metadata));

	do {
		received_bytes = recv(conn_socket, &req_meta->request_data, sizeof(struct request), 0);
		clock_gettime(CLOCK_MONOTONIC, &req_meta->receipt_timestamp);

		/* Don't just return if received_bytes is 0 or -1. Instead
		 * skip the response and break out of the loop in an
		 * orderly fashion so that we can de-allocate the req
		 * and response_data variables, and shutdown the socket. */
		if (received_bytes > 0) {

			/* Handle image registration right away! */
			if(req_meta->request_data.img_op == IMG_REGISTER) {
				clock_gettime(CLOCK_MONOTONIC, &req_meta->start_timestamp);

				register_new_image(conn_socket, &req_meta->request_data);

				clock_gettime(CLOCK_MONOTONIC, &req_meta->completion_timestamp);

				sync_printf("T%ld R%ld:%lf,%s,%d,%ld,%ld,%lf,%lf,%lf\n",
				       conn_config.workers, req_meta->request_data.req_id,
				       TSPEC_TO_DOUBLE(req_meta->request_data.req_timestamp),
				       OPCODE_TO_STRING(req_meta->request_data.img_op),
				       req_meta->request_data.overwrite, req_meta->request_data.img_id,
				       image_count - 1, /* Registered ID on server side */
				       TSPEC_TO_DOUBLE(req_meta->receipt_timestamp),
				       TSPEC_TO_DOUBLE(req_meta->start_timestamp),
				       TSPEC_TO_DOUBLE(req_meta->completion_timestamp));

				dump_queue_status(client_queue);
				continue;
			

				/* Assign a unique img_id */
				uint64_t img_id = next_img_id++;


				// Declare `new_image_data` to hold the received image data
				struct image *new_image_data = recvImage(conn_socket);

				/* Store the image */
				add_image_to_storage(img_id, new_image_data);

				/* Prepare and send the response */
				response_data.req_id = req_meta->request_data.req_id;
				response_data.img_id = img_id;
				response_data.ack = RESP_COMPLETED;
				send(conn_socket, &response_data, sizeof(struct response), 0);

				/* Log the operation */
				clock_gettime(CLOCK_MONOTONIC, &req_meta->start_timestamp);
				clock_gettime(CLOCK_MONOTONIC, &req_meta->completion_timestamp);

				sync_printf("T0 R%ld:%lf,%s,%d,%ld,%ld,%lf,%lf,%lf\n",
					req_meta->request_data.req_id,
					TSPEC_TO_DOUBLE(req_meta->request_data.req_timestamp),
					OPCODE_TO_STRING(req_meta->request_data.img_op),
					req_meta->request_data.overwrite,
					0UL,
					img_id,
					TSPEC_TO_DOUBLE(req_meta->receipt_timestamp),
					TSPEC_TO_DOUBLE(req_meta->start_timestamp),
					TSPEC_TO_DOUBLE(req_meta->completion_timestamp)
				);

				continue; /* Skip adding to queue */
			}	

			result = add_to_queue(*req_meta, client_queue);

			/* The queue is full if the return value is 1 */
			if (result) {
				struct response response_data;
				/* Now provide a response! */
				response_data.req_id = req_meta->request_data.req_id;
				response_data.ack = RESP_REJECTED;
				send(conn_socket, &response_data, sizeof(struct response), 0);

				sync_printf("X%ld:%lf,%lf,%lf\n", req_meta->request_data.req_id,
				       TSPEC_TO_DOUBLE(req_meta->request_data.req_timestamp),
				       TSPEC_TO_DOUBLE(req_meta->request_data.req_length),
				       TSPEC_TO_DOUBLE(req_meta->receipt_timestamp)
					);
			}
		}
	} while (received_bytes > 0);

	/* Stop all the worker threads. */
	control_workers(WORKERS_STOP, conn_config.workers, NULL);

	free(req_meta);
	shutdown(conn_socket, SHUT_RDWR);
	close(conn_socket);
	printf("INFO: Client disconnected.\n");
}


/* Template implementation of the main function for the FIFO
 * server. The server must accept in input a command line parameter
 * with the <port number> to bind the server to. */
int main (int argc, char ** argv) {
	int sockfd, retval, accepted_socket, optval, opt;
	in_port_t socket_port;
	struct sockaddr_in addr, client;
	struct in_addr any_address;
	socklen_t client_len;
	struct connection_config conn_config;
	struct worker_config shared_worker_config;
	conn_config.queue_size = 0;
	conn_config.queue_policy = QUEUE_FIFO;
	conn_config.workers = 1;
	// In your main function, after parsing command-line arguments
	
	shared_worker_config.event_type = conn_config.event_type;


	/*
	TODO: Parse -h flag for what hardware counter to profile
	*/

	/* Parse all the command line arguments */
	while((opt = getopt(argc, argv, "q:w:p:h:")) != -1) {
		switch (opt) {
		case 'q':
			conn_config.queue_size = strtol(optarg, NULL, 10);
			printf("INFO: setting queue size = %ld\n", conn_config.queue_size);
			break;
		case 'w':
			conn_config.workers = strtol(optarg, NULL, 10);
			printf("INFO: setting worker count = %ld\n", conn_config.workers);
			if (conn_config.workers != 1) {
				ERROR_INFO();
				fprintf(stderr, "Only 1 worker is supported in this implementation!\n" USAGE_STRING, argv[0]);
				return EXIT_FAILURE;
			}
			break;
		case 'p':
			if (!strcmp(optarg, "FIFO")) {
				conn_config.queue_policy = QUEUE_FIFO;
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
                conn_config.event_type = EVENT_INSTR;
            } else if (!strcmp(optarg, "L1MISS")) {
                conn_config.event_type = EVENT_L1MISS;
            } else if (!strcmp(optarg, "LLCMISS")) {
                conn_config.event_type = EVENT_LLCMISS;
            } else {
                fprintf(stderr, "Invalid event type specified with -h\n" USAGE_STRING, argv[0]);
                return EXIT_FAILURE;
            }
            printf("INFO: setting hardware event = %s\n", optarg);
            break;

		}
	}

	if (!conn_config.queue_size) {
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
	accepted_socket = accept(sockfd, (struct sockaddr *)&client, &client_len);

	if (accepted_socket == -1) {
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
	handle_client_connection(accepted_socket, conn_config);

	free(queue_mutex);
	free(queue_notify);

	close(sockfd);
	return EXIT_SUCCESS;
}
