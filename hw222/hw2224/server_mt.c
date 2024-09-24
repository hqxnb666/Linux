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

/* Include struct definitions and other libraries that need to be
 * included by both client and server */
#include "common.h"

#define BACKLOG_COUNT 100
#define USAGE_STRING				\
	"Missing parameter. Exiting.\n"		\
	"Usage: %s <port_number>\n"

/* 4KB of stack for the worker thread */
#define STACK_SIZE (4096)

/* Main logic of the worker thread */
void* worker_main(void* arg)
{
    struct timespec timestamp;
    struct timespec delay = {1, 0}; // 1 second delay

    // 获取当前时间并输出 "Worker Thread Alive!"
    clock_gettime(CLOCK_REALTIME, &timestamp);
    printf("[#WORKER#] %.6f Worker Thread Alive!\n", TSPEC_TO_DOUBLE(timestamp));

    while (1)
    {
        // 忙等待 1 秒
        busywait_timespec(delay);

        // 获取当前时间并输出 "Still Alive!"
        clock_gettime(CLOCK_REALTIME, &timestamp);
        printf("[#WORKER#] %.6f Still Alive!\n", TSPEC_TO_DOUBLE(timestamp));

        // 睡眠 1 秒
        nanosleep(&delay, NULL);
    }

    return NULL;
}

/* Main function to handle connection with the client. This function
 * takes in input conn_socket and returns only when the connection
 * with the client is interrupted. */
void handle_connection(int conn_socket)
{
    pthread_t worker_thread;

    /* The connection with the client is alive here. Let's start
     * the worker thread. */

    /* IMPLEMENT HERE THE LOGIC TO START THE WORKER THREAD. */
    if (pthread_create(&worker_thread, NULL, worker_main, NULL) != 0)
    {
        perror("Failed to create worker thread");
        exit(EXIT_FAILURE);
    }

    /* We are ready to proceed with the rest of the request
     * handling logic. */

    /* REUSE LOGIC FROM HW1 TO HANDLE THE PACKETS */
    struct request req;
    ssize_t bytes_received;
    struct timespec current_time;

    while ((bytes_received = recv(conn_socket, &req, sizeof(req), 0)) > 0)
    {
        // 获取当前时间戳
        clock_gettime(CLOCK_REALTIME, &current_time);

        // 输出请求信息
        printf("[#SERVER#] %.6f Received Request ID: %lu, Length: %.6f\n",
               TSPEC_TO_DOUBLE(current_time), req.req_id, TSPEC_TO_DOUBLE(req.req_length));

        // 模拟处理请求的忙等待
        busywait_timespec(req.req_length);

        // 发送响应给客户端
        struct response resp;
        resp.req_id = req.req_id;
        resp.ack = 1;
        send(conn_socket, &resp, sizeof(resp), 0);
    }

    // 关闭客户端套接字
    close(conn_socket);

    // 可根据需要决定是否等待工作线程结束
    // pthread_join(worker_thread, NULL);
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

    /* Ready to handle the new connection with the client. */
    handle_connection(accepted);

    close(sockfd);
    return EXIT_SUCCESS;

}
