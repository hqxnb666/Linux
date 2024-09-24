/*
 * common.h - Common definitions for client and server
 */

#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <math.h>

/* Includes that are specific for TCP/IP */
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */

/* Include our own timelib */
#include "timelib.h"

/* This is a handy definition to print out runtime errors that report
 * the file and line number where the error was encountered. */
#define ERROR_INFO()							\
	fprintf(stderr, "Runtime error at %s:%d\n", __FILE__, __LINE__)

/* Request payload as sent by the client and received by the
 * server. */
struct request {
	uint64_t req_id;
	struct timespec req_timestamp;
	struct timespec req_length;
};

/* Response payload as sent by the server and received by the
 * client. */
struct response {
	uint64_t req_id;
	uint64_t reserved;
	uint8_t ack;
};

#endif /* COMMON_H */

