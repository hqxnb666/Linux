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
#include <pthread.h>

/* Needed for wait(...) */
#include <sys/types.h>
#include <sys/wait.h>

/* Needed for semaphores */
#include <semaphore.h>

/* Include struct definitions and other libraries that need to be
 * included by both client and server */
#include "common.h"
#include "timelib.h"

#define BACKLOG_COUNT 100
#define USAGE_STRING				\
	"Missing parameter. Exiting.\n"		\
	"Usage: %s <port_number>\n"

/* 4KB of stack for the worker thread */
#define STACK_SIZE (4096)

/* START - Variables needed to protect the shared queue. DO NOT TOUCH */
sem_t * queue_mutex;
sem_t * queue_notify;
/* END - Variables needed to protect the shared queue. DO NOT TOUCH */

/* Max number of requests that can be queued */
#define QUEUE_SIZE 500

/* 定义包含请求和接收时间戳的结构体 */
struct request_info {
    struct request req;                 // 原始请求
    struct timespec receipt_timestamp;  // 接收时间戳
};

/* 队列结构 */
struct queue {
    struct request_info requests[QUEUE_SIZE];
    int front; // 队首索引
    int rear;  // 队尾索引
    int size;  // 队列中的元素数量
};

/* 工作线程参数结构 */
struct worker_params {
    struct queue *the_queue;
    int conn_socket;
};

/* 添加新请求到共享队列 */
int add_to_queue(struct request_info to_add, struct queue * the_queue)
{
	int retval = 0;
	/* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
	sem_wait(queue_mutex);
	/* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

	/* 检查队列是否已满 */
	if (the_queue->size >= QUEUE_SIZE) {
	    retval = -1; // 队列已满
	} else {
	    // 将请求添加到队尾
	    the_queue->requests[the_queue->rear] = to_add;
	    the_queue->rear = (the_queue->rear + 1) % QUEUE_SIZE;
	    the_queue->size++;
	    retval = 0; // 成功添加
	}

	/* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
	sem_post(queue_mutex);
	sem_post(queue_notify);
	/* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
	return retval;
}

/* 从共享队列获取请求 */
struct request_info get_from_queue(struct queue * the_queue)
{
	struct request_info retval;
	/* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
	sem_wait(queue_notify);
	sem_wait(queue_mutex);
	/* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

	/* 检查队列是否为空 */
	if (the_queue->size <= 0) {
	    // 队列为空，处理错误（根据需求调整）
	    // 这里假设不会发生，因为已经等待了 queue_notify
	} else {
	    // 从队首获取请求
	    retval = the_queue->requests[the_queue->front];
	    the_queue->front = (the_queue->front + 1) % QUEUE_SIZE;
	    the_queue->size--;
	}

	/* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
	sem_post(queue_mutex);
	/* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
	return retval;
}

/* 输出当前队列状态 */
void dump_queue_status(struct queue * the_queue)
{
	int i, index;
	/* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
	sem_wait(queue_mutex);
	/* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

	/* 输出队列状态 */
	printf("Q:[");
	for (i = 0; i < the_queue->size; i++) {
	    index = (the_queue->front + i) % QUEUE_SIZE;
	    printf("R%lu", the_queue->requests[index].req.req_id);
	    if (i < the_queue->size - 1) {
	        printf(",");
	    }
	}
	printf("]\n");

	/* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
	sem_post(queue_mutex);
	/* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
}

/* 工作线程的主函数 */
void* worker_main(void* arg)
{
    struct worker_params *params = (struct worker_params *)arg;
    struct queue *the_queue = params->the_queue;
    int conn_socket = params->conn_socket;
    struct request_info req_info;
    struct timespec start_time, completion_time;
    struct response resp;

    while (1) {
        /* 从队列中获取请求 */
        req_info = get_from_queue(the_queue);

        /* 获取开始处理时间戳 */
        clock_gettime(CLOCK_REALTIME, &start_time);

        /* 模拟处理请求（忙等待） */
        busywait_timespec(req_info.req.req_length);

        /* 获取完成时间戳 */
        clock_gettime(CLOCK_REALTIME, &completion_time);

        /* 发送响应给客户端 */
        resp.req_id = req_info.req.req_id;
        resp.ack = 1;
        send(conn_socket, &resp, sizeof(resp), 0);

        /* 输出处理完成的信息 */
        printf("R%lu:%.6f,%.6f,%.6f,%.6f,%.6f\n",
               req_info.req.req_id,
               TSPEC_TO_DOUBLE(req_info.req.req_timestamp),        // sent timestamp
               TSPEC_TO_DOUBLE(req_info.req.req_length),           // request length
               TSPEC_TO_DOUBLE(req_info.receipt_timestamp),        // receipt timestamp
               TSPEC_TO_DOUBLE(start_time),                        // start timestamp
               TSPEC_TO_DOUBLE(completion_time));                  // completion timestamp

        /* 输出当前队列状态 */
        dump_queue_status(the_queue);
    }

    return NULL;
}

/* 处理客户端连接的函数 */
void handle_connection(int conn_socket)
{
	struct request req;
	struct request_info req_info;
	struct queue * the_queue;
	ssize_t in_bytes;
    pthread_t worker_thread;
    struct worker_params params;

	/* The connection with the client is alive here. Let's
	 * initialize the shared queue. */

	/* 初始化队列 */
    the_queue = (struct queue *)malloc(sizeof(struct queue));
    the_queue->front = 0;
    the_queue->rear = 0;
    the_queue->size = 0;

	/* Queue ready to go here. Let's start the worker thread. */

	/* 启动工作线程 */
    params.the_queue = the_queue;
    params.conn_socket = conn_socket;
    if (pthread_create(&worker_thread, NULL, worker_main, &params) != 0) {
        perror("Failed to create worker thread");
        exit(EXIT_FAILURE);
    }

	/* We are ready to proceed with the rest of the request
	 * handling logic. */

	/* 接收并处理请求 */
	do {
		in_bytes = recv(conn_socket, &req, sizeof(struct request), 0);
		/* 不要直接返回，如果 in_bytes 为 0 或 -1，应当优雅地退出循环并关闭连接 */
		if (in_bytes > 0) {
			/* 记录接收时间戳 */
			clock_gettime(CLOCK_REALTIME, &(req_info.receipt_timestamp));
			/* 存储请求数据 */
			req_info.req = req;
			/* 将请求添加到队列 */
			add_to_queue(req_info, the_queue);
		}
	} while (in_bytes > 0);

	/* PERFORM ORDERLY DEALLOCATION AND OUTRO HERE */
    // 释放资源
    free(the_queue);

    // 可根据需要决定是否等待工作线程结束
    // pthread_join(worker_thread, NULL);

    close(conn_socket);
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

	/* Get port to bind our socket to */
	if (argc > 1) {
		socket_port = strtol(argv[1], NULL, 10);
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
	handle_connection(accepted);

	free(queue_mutex);
	free(queue_notify);

	close(sockfd);
	return EXIT_SUCCESS;

}

