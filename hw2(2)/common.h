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
#define ERROR_INFO() \
    fprintf(stderr, "Runtime error at %s:%d\n", __FILE__, __LINE__)

/* A simple macro to convert between a struct timespec and a double
 * representation of a timestamp. */
#define TSPEC_TO_DOUBLE(spec) \
    ((double)(spec.tv_sec) + (double)(spec.tv_nsec)/NANO_IN_SEC)

/* Define BACKLOG_COUNT if not already defined */
#ifndef BACKLOG_COUNT
#define BACKLOG_COUNT 100
#endif

/* Define QUEUE_SIZE if not already defined */
#ifndef QUEUE_SIZE
#define QUEUE_SIZE 500
#endif

/* Request payload as sent by the client and received by the
 * server. */
struct request {
    uint64_t req_id;
    struct timespec sent_timestamp;     // 请求发送时间戳
    struct timespec receipt_timestamp;  // 请求接收时间戳
    struct timespec req_length;         // 请求处理长度
    int conn_socket;                    // 连接套接字，用于发送响应
};

/* Response payload as sent by the server and received by the
 * client. */
struct response {
    uint64_t req_id;
    uint64_t reserved;
    uint8_t ack;
};

#endif // COMMON_H

