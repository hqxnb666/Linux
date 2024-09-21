#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080

void *worker_main(void *arg) {
    struct timespec ts;
    while (1) {
        clock_gettime(CLOCK_REALTIME, &ts);
        printf("[#WORKER#] %ld.%09ld Worker Thread Alive!\n", ts.tv_sec, ts.tv_nsec);
        sleep(1);
        clock_gettime(CLOCK_REALTIME, &ts);
        printf("[#WORKER#] %ld.%09ld Still Alive!\n", ts.tv_sec, ts.tv_nsec);
        sleep(1);
    }
}

void handle_connection(int socket) {
    char buffer[1024];
    int bytes_read;
    while ((bytes_read = read(socket, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        printf("Received: %s\n", buffer);
        write(socket, buffer, bytes_read);  // Echo back the received message
    }
    close(socket);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    pthread_t thread_id;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    pthread_create(&thread_id, NULL, worker_main, NULL);

    handle_connection(new_socket);

    
    pthread_join(thread_id, NULL);
    return 0;
}

