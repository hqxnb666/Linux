#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>

#include "common.h"

#define USAGE_STRING \
    "Usage: %s -a <arrival_rate> -s <service_time> -n <num_requests> <port>\n"

int main(int argc, char *argv[]) {
    int sockfd, port, num_requests = 0;
    double arrival_rate = 0.0, service_time = 0.0;
    struct sockaddr_in server_addr;
    char *endptr;
    int opt;

    /* 解析命令行参数 */
    while ((opt = getopt(argc, argv, "a:s:n:")) != -1) {
        switch (opt) {
            case 'a':
                arrival_rate = strtod(optarg, &endptr);
                break;
            case 's':
                service_time = strtod(optarg, &endptr);
                break;
            case 'n':
                num_requests = strtol(optarg, &endptr, 10);
                break;
            default:
                fprintf(stderr, USAGE_STRING, argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Expected port number after options\n");
        exit(EXIT_FAILURE);
    }

    port = atoi(argv[optind]);

    /* 检查参数是否有效 */
    if (arrival_rate <= 0.0) {
        fprintf(stderr, "Invalid arrival rate\n");
        exit(EXIT_FAILURE);
    }
    if (service_time <= 0.0) {
        fprintf(stderr, "Invalid service time\n");
        exit(EXIT_FAILURE);
    }
    if (num_requests <= 0) {
        fprintf(stderr, "Invalid number of requests\n");
        exit(EXIT_FAILURE);
    }

    /* 调试信息 */
    printf("Arrival Rate: %f\n", arrival_rate);
    printf("Service Time: %f\n", service_time);
    printf("Number of Requests: %d\n", num_requests);
    printf("Port: %d\n", port);

    /* 创建套接字 */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    /* 设置服务器地址 */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    /* 将服务器地址设为 localhost */
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    /* 连接到服务器 */
    printf("Connecting to server at 127.0.0.1:%d\n", port);
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed");
        exit(EXIT_FAILURE);
    }

    /* 设置请求参数 */
    struct request req;
    struct response resp;
    struct timespec req_length;
    uint64_t request_id = 0;
    double inter_arrival_time = 1.0 / arrival_rate;

    req_length.tv_sec = (long)service_time;
    req_length.tv_nsec = (long)((service_time - req_length.tv_sec) * 1e9);

    int i;
    /* 发送请求 */
    for (i = 0; i < num_requests; i++) {
        request_id++;

        /* 填充请求结构体 */
        req.request_id = request_id;
        clock_gettime(CLOCK_MONOTONIC, &req.sent_time);
        req.req_length = req_length;

        /* 发送请求 */
        printf("Sending request %lu\n", request_id);
        if (send(sockfd, &req, sizeof(req), 0) < 0) {
            perror("Send failed");
            exit(EXIT_FAILURE);
        }

        /* 接收响应 */
        if (recv(sockfd, &resp, sizeof(resp), 0) <= 0) {
            perror("Receive failed");
            exit(EXIT_FAILURE);
        }

        /* 模拟到达间隔 */
        usleep(inter_arrival_time * 1e6);
    }

    /* 关闭套接字 */
    close(sockfd);

    return EXIT_SUCCESS;
}

