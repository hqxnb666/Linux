#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sched.h>
#include <signal.h>

/* Needed for wait(...) */
#include <sys/types.h>
#include <sys/wait.h>

/* Include struct definitions and other libraries that need to be
 * included by both client and server */
#include "common.h"
#include "timelib.h"
#include <pthread.h>
#include <unistd.h> // For sleep function

#define BACKLOG_COUNT 100
#define USAGE_STRING				\
	"Missing parameter. Exiting.\n"		\
	"Usage: %s <port_number>\n"

/* 4KB of stack for the worker thread */
#define STACK_SIZE (4096)

/* Main logic of the worker thread */
void *worker_main(void *arg)
{
    struct timespec ts;
    double timestamp;

    // 获取当前时间戳并打印 "Worker Thread Alive!" 消息
    clock_gettime(CLOCK_MONOTONIC, &ts); // 修改为 CLOCK_MONOTONIC
    timestamp = TSPEC_TO_DOUBLE(ts);
    printf("[#WORKER#] %.6f Worker Thread Alive!\n", timestamp);

    while (1)
    {
        // 忙等待 1 秒
        struct timespec delay = {1, 0};
        busywait_timespec(delay);

        // 获取当前时间戳并打印 "Still Alive!" 消息
        clock_gettime(CLOCK_MONOTONIC, &ts); // 修改为 CLOCK_MONOTONIC
        timestamp = TSPEC_TO_DOUBLE(ts);
        printf("[#WORKER#] %.6f Still Alive!\n", timestamp);

        // 休眠 1 秒
        sleep(1);
    }

    return NULL;
}

/* Main function to handle connection with the client. This function
 * takes in input conn_socket and returns only when the connection
 * with the client is interrupted. */
void handle_connection(int conn_socket)
{
    pthread_t worker_thread;
    int result;

    /* 连接已建立，启动工作线程 */
    result = pthread_create(&worker_thread, NULL, worker_main, NULL);
    if (result != 0) {
        ERROR_INFO();
        perror("无法创建工作线程");
        exit(EXIT_FAILURE);
    }

    /* 处理客户端请求 */
    struct request *req;
    struct response *resp;
    size_t in_bytes, out_bytes;

    req = (struct request *)malloc(sizeof(struct request));
    resp = (struct response *)malloc(sizeof(struct response));

    do {
        in_bytes = recv(conn_socket, req, sizeof(struct request), 0);

        if (in_bytes > 0) {
            // 获取接收时间戳
            struct timespec receipt_ts;
            clock_gettime(CLOCK_MONOTONIC, &receipt_ts); // 修改为 CLOCK_MONOTONIC

            // 模拟处理请求，忙等待指定的长度
            double req_length = TSPEC_TO_DOUBLE(req->req_length);
            struct timespec delay = req->req_length;

            busywait_timespec(delay);

            // 发送响应
            resp->req_id = req->req_id;
            resp->ack = 1;
            out_bytes = send(conn_socket, resp, sizeof(struct response), 0);
            if (out_bytes < 0) {
                ERROR_INFO();
                perror("发送响应失败");
                break;
            }

            // 获取完成时间戳
           struct timespec completion_ts;
clock_gettime(CLOCK_MONOTONIC, &completion_ts);

            // 打印请求处理报告
            double sent_timestamp = TSPEC_TO_DOUBLE(req->req_timestamp);
            double receipt_timestamp = TSPEC_TO_DOUBLE(receipt_ts);
            double start_timestamp = receipt_timestamp; // 假设处理开始时间与接收时间相同
            double completion_timestamp = TSPEC_TO_DOUBLE(completion_ts);

           printf("R%lu:%ld.%09ld,%ld.%09ld,%ld.%09ld,%ld.%09ld,%ld.%09ld,%ld.%09ld\n",
       req->req_id,
       req->req_timestamp.tv_sec, req->req_timestamp.tv_nsec, // 发送时间戳
       req->req_length.tv_sec, req->req_length.tv_nsec,       // 请求长度
       receipt_ts.tv_sec, receipt_ts.tv_nsec,                 // 接收时间戳
       start_timestamp.tv_sec, start_timestamp.tv_nsec,       // 开始处理时间戳
       completion_ts.tv_sec, completion_ts.tv_nsec); 

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

    // 清理资源
    free(req);
    free(resp);
    close(conn_socket);

    // 如果需要，可以在此处取消并等待工作线程结束
    // pthread_cancel(worker_thread);
    // pthread_join(worker_thread, NULL);
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
		perror("无法创建套接字");
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

	retval = bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

	if (retval < 0) {
		ERROR_INFO();
		perror("无法绑定套接字");
		return EXIT_FAILURE;
	}

	/* 开始监听 */
	retval = listen(sockfd, BACKLOG_COUNT);

	if (retval < 0) {
		ERROR_INFO();
		perror("无法监听套接字");
		return EXIT_FAILURE;
	}

	/* 准备接受连接 */
	printf("INFO: Waiting for incoming connection...\n");
	client_len = sizeof(struct sockaddr_in);
	accepted = accept(sockfd, (struct sockaddr *)&client, &client_len);

	if (accepted == -1) {
		ERROR_INFO();
		perror("无法接受连接");
		return EXIT_FAILURE;
	}

	/* 处理新连接 */
	handle_connection(accepted);

	close(sockfd);
	return EXIT_SUCCESS;

}
