#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "timelib.h"

void handle_connection(int client_sock);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    int port = atoi(argv[1]);
    socklen_t addr_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Binding failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 5) == -1) {
        perror("Listening failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", port);

    while ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len)) != -1) {
        printf("Accepted a connection...\n");
        handle_connection(client_sock);
        close(client_sock);
    }

    perror("Accepting connection failed");
    close(server_sock);
    return 0;
}

void handle_connection(int client_sock) {
    while (1) {
        uint64_t request_id;
        struct timespec sent_timestamp;
        struct timespec request_length;

        ssize_t bytes_received = recv(client_sock, &request_id, sizeof(request_id), MSG_WAITALL);
        if (bytes_received == 0) {
            break;
        } else if (bytes_received < 0) {
            perror("Failed to receive request_id");
            break;
        }

        bytes_received = recv(client_sock, &sent_timestamp, sizeof(sent_timestamp), MSG_WAITALL);
        if (bytes_received <= 0) {
            perror("Failed to receive sent_timestamp");
            break;
        }

        bytes_received = recv(client_sock, &request_length, sizeof(request_length), MSG_WAITALL);
        if (bytes_received <= 0) {
            perror("Failed to receive request_length");
            break;
        }

        struct timespec receipt_timestamp;
        clock_gettime(CLOCK_MONOTONIC, &receipt_timestamp);

        get_elapsed_busywait(request_length.tv_sec, request_length.tv_nsec);

        struct timespec completion_timestamp;
        clock_gettime(CLOCK_MONOTONIC, &completion_timestamp);

        uint64_t reserved_field = 0;
        uint8_t ack_value = 0;

        ssize_t bytes_sent = send(client_sock, &request_id, sizeof(request_id), 0);
        if (bytes_sent <= 0) {
            perror("Failed to send request_id");
            break;
        }

        bytes_sent = send(client_sock, &reserved_field, sizeof(reserved_field), 0);
        if (bytes_sent <= 0) {
            perror("Failed to send reserved_field");
            break;
        }

        bytes_sent = send(client_sock, &ack_value, sizeof(ack_value), 0);
        if (bytes_sent <= 0) {
            perror("Failed to send ack_value");
            break;
        }
        
        printf("R%lu:", request_id);
        printf("%ld.%06ld,", sent_timestamp.tv_sec, sent_timestamp.tv_nsec / 1000);
        printf("%ld.%06ld,", request_length.tv_sec, request_length.tv_nsec / 1000);
        printf("%ld.%06ld,", receipt_timestamp.tv_sec, receipt_timestamp.tv_nsec / 1000);
        printf("%ld.%06ld\n", completion_timestamp.tv_sec, completion_timestamp.tv_nsec / 1000);
    }
}

