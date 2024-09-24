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
#include <pthread.h>
#include <unistd.h>
#include <string.h>

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

/* 定义请求队列的数据结构 */
struct queue {
    struct request requests[QUEUE_SIZE];
    int front; // 队头索引
    int rear;  // 队尾索引
    int size;  // 队列中的元素数量
};

/* 添加新请求到共享队列 */
int add_to_queue(struct request to_add, struct queue * the_queue)
{
    int retval = 0;
    /* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
    sem_wait(queue_mutex);
    /* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

    /* 检查队列是否已满 */
    if (the_queue->size >= QUEUE_SIZE) {
        retval = -1; // 队列已满
        printf("Queue is full, dropping request R%lu\n", to_add.req_id);
    } else {
        // 将请求添加到队列
        the_queue->requests[the_queue->rear] = to_add;
        the_queue->rear = (the_queue->rear + 1) % QUEUE_SIZE;
        the_queue->size++;
        sem_post(queue_notify); // 通知工作线程有新请求
    }

    /* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
    sem_post(queue_mutex);
    /* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
    return retval;
}


/* 从共享队列中获取请求 */
struct request get_from_queue(struct queue * the_queue)
{
	struct request retval;
	/* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
	sem_wait(queue_notify);
	sem_wait(queue_mutex);
	/* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

	/* 检查队列是否为空 */
	if (the_queue->size <= 0) {
		// 队列为空，不应该发生
		printf("Queue is empty, but get_from_queue was called.\n");
		memset(&retval, 0, sizeof(struct request));
	} else {
		// 从队列中取出请求
		retval = the_queue->requests[the_queue->front];
		the_queue->front = (the_queue->front + 1) % QUEUE_SIZE;
		the_queue->size--;
	}

	/* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
	sem_post(queue_mutex);
	/* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
	return retval;
}

/* 打印队列状态 */
void dump_queue_status(struct queue * the_queue)
{
	int i, index;
	/* QUEUE PROTECTION INTRO START --- DO NOT TOUCH */
	sem_wait(queue_mutex);
	/* QUEUE PROTECTION INTRO END --- DO NOT TOUCH */

	printf("Q:[");
	for (i = 0; i < the_queue->size; i++) {
		index = (the_queue->front + i) % QUEUE_SIZE;
		printf("R%lu", the_queue->requests[index].req_id);
		if (i != the_queue->size - 1) {
			printf(",");
		}
	}
	printf("]\n");

	/* QUEUE PROTECTION OUTRO START --- DO NOT TOUCH */
	sem_post(queue_mutex);
	/* QUEUE PROTECTION OUTRO END --- DO NOT TOUCH */
}

/* 工作线程的主函数 */
void *worker_main(void *arg)
{
    struct queue *the_queue = (struct queue *)arg;
    struct request req;

    while (1) {
        // 从队列中获取请求
        req = get_from_queue(the_queue);

        // 检查是否是特殊的终止请求
        if (req.req_id == UINT64_MAX) {
            // 退出循环
            break;
        }

        // 获取开始处理时间戳
        struct timespec start_ts;
        clock_gettime(CLOCK_MONOTONIC, &start_ts);

        // 模拟处理请求（忙等待）
        busywait_timespec(req.req_length);

        // 获取完成时间戳
        struct timespec completion_ts;
        clock_gettime(CLOCK_MONOTONIC, &completion_ts);

        // 打印请求处理报告
        printf("R%lu:%ld.%09ld,%ld.%09ld,%ld.%09ld,%ld.%09ld,%ld.%09ld\n",
               req.req_id,
               req.req_timestamp.tv_sec, req.req_timestamp.tv_nsec, // 发送时间戳
               req.req_length.tv_sec, req.req_length.tv_nsec,       // 请求长度
               req.req_timestamp.tv_sec, req.req_timestamp.tv_nsec, // 接收时间戳
               start_ts.tv_sec, start_ts.tv_nsec,                   // 开始处理时间戳
               completion_ts.tv_sec, completion_ts.tv_nsec);        // 完成时间戳

        // 打印队列状态
        dump_queue_status(the_queue);
    }

    return NULL;
}


/* 处理与客户端的连接 */
void handle_connection(int conn_socket)
{
	struct request * req;
	struct queue * the_queue;
	size_t in_bytes;

	/* 连接已建立，初始化共享队列 */
	the_queue = (struct queue *)malloc(sizeof(struct queue));
	the_queue->front = 0;
	the_queue->rear = 0;
	the_queue->size = 0;

	/* 创建工作线程 */
	pthread_t worker_thread;
	int result;
	result = pthread_create(&worker_thread, NULL, worker_main, (void *)the_queue);
	if (result != 0) {
		ERROR_INFO();
		perror("无法创建工作线程");
		exit(EXIT_FAILURE);
	}

	/* 处理客户端请求 */
	req = (struct request *)malloc(sizeof(struct request));

	do {
		in_bytes = recv(conn_socket, req, sizeof(struct request), 0);

		if (in_bytes > 0) {
			// 获取接收时间戳
			clock_gettime(CLOCK_MONOTONIC, &req->req_timestamp); // 修改为 CLOCK_MONOTONIC

			// 将请求添加到队列
			add_to_queue(*req, the_queue);
		} else if (in_bytes == 0) {
			// 客户端关闭连接
			break;
		} else {
			// 发生错误
			ERROR_INFO();
			perror("接收请求失败");
			break;
		}
	} while (in_bytes > 0);

	// 通知工作线程退出
struct request exit_req;
exit_req.req_id = UINT64_MAX; // 使用最大值表示终止请求
exit_req.req_length.tv_sec = 0;
exit_req.req_length.tv_nsec = 0;
exit_req.req_timestamp.tv_sec = 0;
exit_req.req_timestamp.tv_nsec = 0;

add_to_queue(exit_req, the_queue);

// 等待工作线程结束
pthread_join(worker_thread, NULL);


	/* 释放资源 */
	free(req);
	free(the_queue);
	close(conn_socket);
}

/* 主函数，接受命令行参数 <port_number>，并启动服务器 */
int main (int argc, char ** argv) {
	int sockfd, retval, accepted, optval;
	in_port_t socket_port;
	struct sockaddr_in addr, client;
	struct in_addr any_address;
	socklen_t client_len;

	/* 获取要绑定的端口号 */
	if (argc > 1) {
		socket_port = strtol(argv[1], NULL, 10);
		printf("INFO: setting server port as: %d\n", socket_port);
	} else {
		ERROR_INFO();
		fprintf(stderr, USAGE_STRING, argv[0]);
		return EXIT_FAILURE;
	}

	/* 创建套接字 */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		ERROR_INFO();
		perror("Unable to create socket");
		return EXIT_FAILURE;
	}

	/* 设置套接字地址可重用 */
	optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&optval, sizeof(optval));

	/* 将 INADDR_ANY 转换为网络字节序 */
	any_address.s_addr = htonl(INADDR_ANY);

	/* 绑定套接字到指定端口 */
	addr.sin_family = AF_INET;
	addr.sin_port = htons(socket_port);
	addr.sin_addr = any_address;

	/* 尝试绑定套接字 */
	retval = bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

	if (retval < 0) {
		ERROR_INFO();
		perror("Unable to bind socket");
		return EXIT_FAILURE;
	}

	/* 开始监听 */
	retval = listen(sockfd, BACKLOG_COUNT);

	if (retval < 0) {
		ERROR_INFO();
		perror("Unable to listen on socket");
		return EXIT_FAILURE;
	}

	/* 初始化队列保护变量。DO NOT TOUCH */
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
	/* DONE - 初始化队列保护变量。DO NOT TOUCH */

	/* 准备接受连接 */
	printf("INFO: Waiting for incoming connection...\n");
	client_len = sizeof(struct sockaddr_in);
	accepted = accept(sockfd, (struct sockaddr *)&client, &client_len);

	if (accepted == -1) {
		ERROR_INFO();
		perror("Unable to accept connections");
		return EXIT_FAILURE;
	}

	/* 处理新连接 */
	handle_connection(accepted);

	/* 销毁信号量并释放内存 */
	sem_destroy(queue_mutex);
	sem_destroy(queue_notify);
	free(queue_mutex);
	free(queue_notify);

	close(sockfd);
	return EXIT_SUCCESS;

}
