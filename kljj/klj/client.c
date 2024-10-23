#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "timelib.h"
#include "common.h"

#define DEFAULT_PORT 2222

void print_usage(char *prog_name) {
    fprintf(stderr, "Usage: %s -a <arrival_rate> -s <service_time> -n <num_requests> <server_ip> [port]\n", prog_name);
}

int main(int argc, char *argv[]) {
    int sockfd, portno, opt;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    double arrival_rate = 0;
    double service_time = 0;
    int num_requests = 0;

    if (argc < 2) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    while ((opt = getopt(argc, argv, "a:s:n:")) != -1) {
        switch (opt) {
            case 'a':
                arrival_rate = atof(optarg);
                break;
            case 's':
                service_time = atof(optarg);
                break;
            case 'n':
                num_requests = atoi(optarg);
                break;
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_ip = argv[optind++];
    portno = (optind < argc) ? atoi(argv[optind]) : DEFAULT_PORT;

    if (arrival_rate <= 0 || service_time <= 0 || num_requests <= 0) {
        fprintf(stderr, "Error: Arrival rate, service time, and number of requests must be positive numbers.\n");
        exit(EXIT_FAILURE);
    }

    /* 创建套接字 */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        exit(EXIT_FAILURE);
    }

    /* 获取服务器地址 */
    server = gethostbyname(server_ip);
    if (server == NULL) {
        fprintf(stderr, "Error: No such host %s\n", server_ip);
        exit(EXIT_FAILURE);
    }

    /* 设置服务器地址结构 */
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *)&serv_addr.sin_addr.s_addr,
           (char *)server->h_addr_list[0],
           server->h_length);
    serv_addr.sin_port = htons(portno);

    /* 连接到服务器 */
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error connecting to server");
        exit(EXIT_FAILURE);
    }

    /* 设置随机数种子 */
    srand(time(NULL));

    /* 计算请求间隔时间（指数分布） */
    double lambda = arrival_rate;
    double mu = 1.0 / service_time;

    struct request req;
    struct response resp;
    uint64_t req_id = 0;
    {
      int i = 0;
    for ( i = 0; i < num_requests; ++i) {
        /* 等待下一个请求的到来 */
        double u = (double)rand() / RAND_MAX;
        double inter_arrival_time = -log(1 - u) / lambda;
        struct timespec sleep_time = dtotspec(inter_arrival_time);
        nanosleep(&sleep_time, NULL);

        /* 准备请求 */
        req.req_id = req_id++;
        clock_gettime(CLOCK_MONOTONIC, &req.req_timestamp);

        /* 生成请求长度（服务时间），假设为指数分布 */
        u = (double)rand() / RAND_MAX;
        double service_duration = -log(1 - u) / mu;
        req.req_length = dtotspec(service_duration);

        /* 发送请求 */
        if (send(sockfd, &req, sizeof(req), 0) < 0) {
            perror("Error sending request");
            exit(EXIT_FAILURE);
        }

        /* 接收响应 */
        if (recv(sockfd, &resp, sizeof(resp), 0) < 0) {
            perror("Error receiving response");
            exit(EXIT_FAILURE);
        }

        /* 处理响应 */
        if (resp.ack == RESP_COMPLETED) {
            printf("Request %ld completed\n", resp.req_id);
        } else if (resp.ack == RESP_REJECTED) {
            printf("Request %ld rejected\n", resp.req_id);
        } else {
            printf("Request %ld unknown response\n", resp.req_id);
        }
    }
    }
    /* 关闭套接字 */
    close(sockfd);

    return 0;
}

