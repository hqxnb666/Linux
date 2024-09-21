/*******************************************************************************
 * Simple Client for Testing Multi-threaded FIFO Server
 *
 * Description:
 *     A simple client program to connect to the server, send a request, and receive
 *     a response. The request includes a request ID and a simulated processing time.
 *
 * Usage:
 *     ./client <server_ip> <port_number> <request_id> <processing_time_sec> <processing_time_nsec>
 *
 * Parameters:
 *     server_ip            - The IP address of the server to connect to.
 *     port_number          - The port number of the server to connect to.
 *     request_id           - A unique identifier for the request.
 *     processing_time_sec  - The simulated processing time in seconds.
 *     processing_time_nsec - The simulated processing time in nanoseconds.
 *
 * Example:
 *     ./client 127.0.0.1 8080 1 1 0
 *
 * Author:
 *     Your Name
 *
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>

/* Include shared structures */
#include "common.h"

/* Function to print usage instructions */
void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s <server_ip> <port_number> <request_id> <processing_time_sec> <processing_time_nsec>\n", prog_name);
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    uint64_t req_id = strtoull(argv[3], NULL, 10);
    long proc_sec = atol(argv[4]);
    long proc_nsec = atol(argv[5]);

    /* Create socket */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket failed");
        return EXIT_FAILURE;
    }

    /* Set server address */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if(inet_pton(AF_INET, server_ip, &server_addr.sin_addr)<=0) {
        perror("Invalid address/ Address not supported");
        close(sockfd);
        return EXIT_FAILURE;
    }

    /* Connect to server */
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed");
        close(sockfd);
        return EXIT_FAILURE;
    }

    /* Prepare request */
    struct request req;
    req.req_id = req_id;
    clock_gettime(CLOCK_REALTIME, &req.sent_timestamp);
    req.req_length.tv_sec = proc_sec;
    req.req_length.tv_nsec = proc_nsec;
    req.conn_socket = sockfd; // Set connection socket

    /* Send request */
    ssize_t sent_bytes = send(sockfd, &req, sizeof(req), 0);
    if (sent_bytes != sizeof(req)) {
        perror("send failed");
        close(sockfd);
        return EXIT_FAILURE;
    }
    printf("Sent Request ID: %lu\n", req.req_id);

    /* Receive response */
    struct response resp;
    ssize_t recv_bytes = recv(sockfd, &resp, sizeof(resp), 0);
    if (recv_bytes < 0) {
        perror("recv failed");
        close(sockfd);
        return EXIT_FAILURE;
    } else if (recv_bytes == 0) {
        printf("Server closed the connection.\n");
        close(sockfd);
        return EXIT_FAILURE;
    }

    /* Print response */
    printf("Received Response for Request ID: %lu, Ack: %d\n", resp.req_id, resp.ack);

    /* Close socket */
    close(sockfd);
    return EXIT_SUCCESS;
}

