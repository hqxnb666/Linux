#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>

/* Include struct definitions and other libraries that need to be
 * included by both client and server */
#include "common.h"
#include "timelib.h"

#define BACKLOG_COUNT 100
#define USAGE_STRING \
    "Missing parameter. Exiting.\n" \
    "Usage: %s <port_number>\n"

static void handle_connection(int conn_socket)
{
    struct request req;
    struct response rep;
    struct timespec start, end;
    ssize_t rec;

    while (1) {
        rec = recv(conn_socket, &req, sizeof(struct request), 0);
        if (rec <= 0) {
            break;
        }

        rep.reserved = 0;
        rep.ack = 0;
        rep.request_id = req.request_id;

        clock_gettime(CLOCK_MONOTONIC, &start);

        get_elapsed_busywait(req.req_length.tv_sec, req.req_length.tv_nsec);

        send(conn_socket, &rep, sizeof(struct response), 0);

        clock_gettime(CLOCK_MONOTONIC, &end);

        double sent = req.sent_time.tv_sec + req.sent_time.tv_nsec / 1e9;
        double request = req.req_length.tv_sec + req.req_length.tv_nsec / 1e9;
        double receipt = start.tv_sec + start.tv_nsec / 1e9;
        double completion = end.tv_sec + end.tv_nsec / 1e9;

        printf("R%ld:%.9f,%.9f,%.9f,%.9f\n", req.request_id, sent, request, receipt, completion);
    }

    close(conn_socket);
}

int main(int argc, char** argv)
{
    int sockfd, retval, accepted, optval;
    in_port_t socket_port;
    struct sockaddr_in addr, client;
    socklen_t client_len;

    if (argc > 1) {
        socket_port = strtol(argv[1], NULL, 10);
        printf("INFO: setting server port as: %d\n", socket_port);
    }
    else {
        ERROR_INFO();
        fprintf(stderr, USAGE_STRING, argv[0]);
        return EXIT_FAILURE;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        ERROR_INFO();
        perror("Unable to create socket");
        return EXIT_FAILURE;
    }

    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void*)&optval, sizeof(optval));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(socket_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    retval = bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));

    if (retval < 0) {
        ERROR_INFO();
        perror("Unable to bind socket");
        close(sockfd);
        return EXIT_FAILURE;
    }

    retval = listen(sockfd, BACKLOG_COUNT);

    if (retval < 0) {
        ERROR_INFO();
        perror("Unable to listen on socket");
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("INFO: Waiting for incoming connection...\n");
    client_len = sizeof(struct sockaddr_in);
    accepted = accept(sockfd, (struct sockaddr*)&client, &client_len);

    if (accepted == -1) {
        ERROR_INFO();
        perror("Unable to accept connections");
        close(sockfd);
        return EXIT_FAILURE;
    }

    handle_connection(accepted);

    close(sockfd);
    return EXIT_SUCCESS;
}
