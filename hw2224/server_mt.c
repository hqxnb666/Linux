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

/* Needed for wait(...) */
#include <sys/types.h>
#include <sys/wait.h>

/* Include struct definitions and other libraries that need to be
 * included by both client and server */
#include "common.h"
#include <pthread.h>

#define BACKLOG_COUNT 100
#define USAGE_STRING				\
	"Missing parameter. Exiting.\n"		\
	"Usage: %s <port_number>\n"

/* 4KB of stack for the worker thread */
#define STACK_SIZE (4096)

int omega = 0;

/* Main logic of the worker thread */
/* IMPLEMENT HERE THE MAIN FUNCTION OF THE WORKER */
void *delta_function(void *epsilon){
  (void)epsilon;

	struct timespec kappa;
	//int sigma = *(int*)epsilon;
	//free(epsilon);

	clock_gettime(CLOCK_MONOTONIC, &kappa);
	double theta = kappa.tv_sec + (double)kappa.tv_nsec / 1e9;
	printf("[#WORKER#] %.9f Worker Thread Alive!\n", theta);

	while(omega != 1){
		
		get_elapsed_busywait(1, 0);
		clock_gettime(CLOCK_MONOTONIC, &kappa);
		double lambda = kappa.tv_sec + (double)kappa.tv_nsec / 1e9;
		printf("[#WORKER#] %.9f Still Alive!\n", lambda);
		get_elapsed_sleep(1, 0);
	}
	//close(sigma);
	return NULL;
}

/* Main function to handle connection with the client. This function
 * takes in input sigma and returns only when the connection
 * with the client is interrupted. */
void handle_connection(int sigma)
{

	/* The connection with the client is alive here. Let's start
	 * the worker thread. */

	/* IMPLEMENT HERE THE LOGIC TO START THE WORKER THREAD. */
	FILE *length_file = fopen("request_lengths.txt", "w");
if (length_file == NULL) {
    perror("Failed to open file");
    // 处理错误
}
  pthread_t mu_thread;
	void *phi_ptr = (void*) &sigma;
	
	
	if (pthread_create(&mu_thread, NULL, delta_function, phi_ptr) != 0) {
		//free(phi_ptr);
		close(sigma);
		return;
	}

	// if (pthread_join(mu_thread, NULL) != 0) {
    // 	perror("Failed to join thread");
    // 	return 1;
    // }

	/* We are ready to proceed with the rest of the request
	 * handling logic. */

	/* REUSE LOGIC FROM HW1 TO HANDLE THE PACKETS */

	struct request psi;
	struct response chi;
    struct timespec alpha, beta;
    int zeta;

    while(1){
	    zeta = recv(sigma, &psi, sizeof(struct request), 0);
	    if (zeta <= 0){
		    break;
	    }
       double request_length = psi.req_length.tv_sec + (double)psi.req_length.tv_nsec / 1e9;
        fprintf(length_file, "%.9f\n", request_length);
	    chi.reserved = 0;
	    chi.ack = 0;
	    chi.req_id = psi.req_id;  
        
        clock_gettime(CLOCK_MONOTONIC, &alpha);

        get_elapsed_busywait(psi.req_length.tv_sec, psi.req_length.tv_nsec);

	    send(sigma, &chi, sizeof(struct response), 0);

        clock_gettime(CLOCK_MONOTONIC, &beta); 

	    double eta = (double)psi.req_timestamp.tv_sec + ((double)psi.req_timestamp.tv_nsec)/1e9;
	    double iota = (double)psi.req_length.tv_sec + ((double)psi.req_length.tv_nsec)/1e9;
	    double nu = (double)alpha.tv_sec + ((double)alpha.tv_nsec)/1e9;
	    double xi = (double)beta.tv_sec + ((double)beta.tv_nsec)/1e9;
    	
        printf("R%ld:%.9f,%.9f,%.9f,%.9f\n", psi.req_id, eta, iota, nu, xi);
	}

	
	omega = 1;
	if (pthread_join(mu_thread, NULL) != 0) {
        perror("Failed to join thread");
        return;
    }
	fclose(length_file);
	close(sigma);


}


/* Template implementation of the main function for the FIFO
 * server. The server must accept in input a command line parameter
 * with the <port number> to bind the server to. */
int main (int argc, char ** argv) {
	int delta_sock, epsilon, phi, gamma;
	in_port_t socket_port;
	struct sockaddr_in rho, tau;
	struct in_addr xi;
	socklen_t kappa_len;

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
	delta_sock = socket(AF_INET, SOCK_STREAM, 0);

	if (delta_sock < 0) {
		ERROR_INFO();
		perror("Unable to create socket");
		return EXIT_FAILURE;
	}

	/* Before moving forward, set socket to reuse address */
	gamma = 1;
	setsockopt(delta_sock, SOL_SOCKET, SO_REUSEADDR, (void *)&gamma, sizeof(gamma));

	/* Convert INADDR_ANY into network byte order */
	xi.s_addr = htonl(INADDR_ANY);

	/* Time to bind the socket to the right port  */
	rho.sin_family = AF_INET;
	rho.sin_port = htons(socket_port);
	rho.sin_addr = xi;

	/* Attempt to bind the socket with the given parameters */
	epsilon = bind(delta_sock, (struct sockaddr *)&rho, sizeof(struct sockaddr_in));

	if (epsilon < 0) {
		ERROR_INFO();
		perror("Unable to bind socket");
		return EXIT_FAILURE;
	}

	/* Let us now proceed to set the server to listen on the selected port */
	epsilon = listen(delta_sock, BACKLOG_COUNT);

	if (epsilon < 0) {
		ERROR_INFO();
		perror("Unable to listen on socket");
		return EXIT_FAILURE;
	}

	/* Ready to accept connections! */
	printf("INFO: Waiting for incoming connection...\n");
	kappa_len = sizeof(struct sockaddr_in);
	phi = accept(delta_sock, (struct sockaddr *)&tau, &kappa_len);

	if (phi == -1) {
		ERROR_INFO();
		perror("Unable to accept connections");
		return EXIT_FAILURE;
	}

	/* Ready to handle the new connection with the client. */
	handle_connection(phi);

	close(delta_sock);
	return EXIT_SUCCESS;

}
