// client.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>

// 定义NANO_IN_SEC，用于时间转换
#define NANO_IN_SEC (1000000000L)

// 请求结构体
struct request {
    uint64_t req_id;
    struct timespec req_timestamp;
    struct timespec req_length;
};

// 响应结构体
struct response {
    uint64_t req_id;
    uint64_t reserved;
    uint8_t ack;
};

// 宏定义，用于错误处理
#define ERROR_EXIT(msg)            \
    do {                           \
        perror(msg);               \
        exit(EXIT_FAILURE);        \
    } while (0)

int main(int argc, char *argv[]) {
    int opt;
    int arrival_time = 0;    // 参数 -a
    int service_time = 0;    // 参数 -s
    int num_requests = 0;    // 参数 -n
    char *server_ip = "127.0.0.1"; // 假设服务器在本地主机
    int server_port = 0;

    // 解析命令行参数
    while ((opt = getopt(argc, argv, "a:s:n:")) != -1) {
        switch (opt) {
            case 'a':
                arrival_time = atoi(optarg);
                break;
            case 's':
                service_time = atoi(optarg);
                break;
            case 'n':
                num_requests = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s -a arrival_time -s service_time -n num_requests <server_port>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Expected server port after options\n");
        fprintf(stderr, "Usage: %s -a arrival_time -s service_time -n num_requests <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    server_port = atoi(argv[optind]);

    if (arrival_time <= 0 || service_time <= 0 || num_requests <= 0 || server_port <= 0) {
        fprintf(stderr, "Invalid arguments. All parameters must be positive integers.\n");
        fprintf(stderr, "Usage: %s -a arrival_time -s service_time -n num_requests <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Client Parameters:\n");
    printf("Arrival Time: %d sec\n", arrival_time);
    printf("Service Time: %d sec\n", service_time);
    printf("Number of Requests: %d\n", num_requests);
    printf("Server Port: %d\n", server_port);

    // 创建TCP套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        ERROR_EXIT("socket");
    }

    // 设置服务器地址结构
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    // 将服务器IP地址从文本转换为二进制形式
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        ERROR_EXIT("inet_pton");
    }

    // 连接到服务器
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ERROR_EXIT("connect");
    }

    printf("Connected to server %s:%d\n", server_ip, server_port);

    // 变量用于计算总响应时间
    double total_response_time = 0.0;

    // 发送请求
    for (int i = 0; i < num_requests; i++) {
        struct request req;
        req.req_id = i;

        // 获取当前时间作为请求时间戳
        if (clock_gettime(CLOCK_MONOTONIC, &req.req_timestamp) != 0) {
            ERROR_EXIT("clock_gettime");
        }

        // 设置请求长度（处理时间）
        req.req_length.tv_sec = service_time;
        req.req_length.tv_nsec = 0;

        // 记录发送时间
        struct timespec send_time;
        clock_gettime(CLOCK_MONOTONIC, &send_time);

        // 发送请求到服务器
        ssize_t sent_bytes = send(sockfd, &req, sizeof(req), 0);
        if (sent_bytes != sizeof(req)) {
            fprintf(stderr, "Failed to send request %d: sent %zd bytes\n", i, sent_bytes);
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // 接收服务器的响应
        struct response res;
        ssize_t recv_bytes = recv(sockfd, &res, sizeof(res), 0);
        if (recv_bytes < 0) {
            ERROR_EXIT("recv");
        } else if (recv_bytes == 0) {
            fprintf(stderr, "Server closed connection unexpectedly.\n");
            close(sockfd);
            exit(EXIT_FAILURE);
        } else if (recv_bytes != sizeof(res)) {
            fprintf(stderr, "Incomplete response received for request %d: received %zd bytes\n", i, recv_bytes);
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // 记录接收时间
        struct timespec recv_time;
        clock_gettime(CLOCK_MONOTONIC, &recv_time);

        // 计算响应时间
        double response_time = (recv_time.tv_sec - send_time.tv_sec) +
                               (recv_time.tv_nsec - send_time.tv_nsec) / 1e9;

        total_response_time += response_time;

        // 验证响应
        if (res.req_id != req.req_id || res.ack != 0) {
            fprintf(stderr, "Invalid response for request %d: req_id=%ld, ack=%d\n", i, res.req_id, res.ack);
        }

        // 输出响应信息（可选）
        /*
        printf("Received response for request %d: req_id=%ld, ack=%d, response_time=%.6f sec\n",
               i, res.req_id, res.ack, response_time);
        */

        // 模拟请求到达间隔时间
        if (i < num_requests - 1) { // 最后一个请求后无需等待
            struct timespec sleep_time;
            sleep_time.tv_sec = arrival_time;
            sleep_time.tv_nsec = 0;

            if (nanosleep(&sleep_time, NULL) != 0) {
                perror("nanosleep");
                // 如果被信号中断，可以选择继续或退出
            }
        }
    }

    // 计算平均响应时间
    double average_response_time = total_response_time / num_requests;
    printf("Average Response Time: %.6f sec\n", average_response_time);

    printf("All %d requests sent and responses received.\n", num_requests);

    // 关闭套接字
    close(sockfd);
    return 0;
}

