#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include "timelib.h"

struct request_t {
    uint64_t request_id;
    struct timespec sent_timestamp;
    struct timespec request_length;
};

struct response_t {
    uint64_t request_id;
    uint64_t reserved;
    uint8_t ack;
};

void handle_connection(int client_socket);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    int port = atoi(argv[1]);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", port);
    while (1) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        printf("Accepted new connection\n");
        handle_connection(client_socket);
        close(client_socket);
        printf("Connection closed\n");
    }

    return 0;
}

void handle_connection(int client_socket) {
    while (1) {
        struct request_t request;
        ssize_t bytes_received = 0;
        size_t total_received = 0;
        size_t expected_size = sizeof(struct request_t);
        char *request_ptr = (char *)&request;

        while (total_received < expected_size) {
            bytes_received = read(client_socket, request_ptr + total_received, expected_size - total_received);
            if (bytes_received == 0) {
                return;
            } else if (bytes_received < 0) {
                perror("read");
                return;
            }
            total_received += bytes_received;
        }

        struct timespec receipt_timestamp;
        clock_gettime(CLOCK_REALTIME, &receipt_timestamp);

        struct timespec *length = &request.request_length;
        get_elapsed_busywait(length->tv_sec, length->tv_nsec);

        struct timespec completion_timestamp;
        clock_gettime(CLOCK_REALTIME, &completion_timestamp);

        struct response_t response;
        response.request_id = request.request_id;
        response.reserved = 0;
        response.ack = 0;

        ssize_t bytes_sent = 0;
        size_t total_sent = 0;
        size_t response_size = sizeof(struct response_t);
        char *response_ptr = (char *)&response;

        while (total_sent < response_size) {
            bytes_sent = write(client_socket, response_ptr + total_sent, response_size - total_sent);
            if (bytes_sent <= 0) {
                perror("write");
                return;
            }
            total_sent += bytes_sent;
        }

        double sent_timestamp = request.sent_timestamp.tv_sec + request.sent_timestamp.tv_nsec / 1e9;
        double request_length = request.request_length.tv_sec + request.request_length.tv_nsec / 1e9;
        double receipt_timestamp_d = receipt_timestamp.tv_sec + receipt_timestamp.tv_nsec / 1e9;
        double completion_timestamp_d = completion_timestamp.tv_sec + completion_timestamp.tv_nsec / 1e9;

        printf("R%lu:%.6f,%.6f,%.6f,%.6f\n", request.request_id, sent_timestamp, request_length, receipt_timestamp_d, completion_timestamp_d);
    }
}

