#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <math.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "timelib.h"

#define RESP_COMPLETED  0
#define RESP_REJECTED   1

#define ERROR_INFO() \
    do { \
        fprintf(stdout, "Runtime error at %s:%d\n", __FILE__, __LINE__); \
        fflush(stdout); \
    } while(0)

#define TSPEC_TO_DOUBLE(t) \
    ((double)(t.tv_sec) + (double)(t.tv_nsec)/NANO_IN_SEC)

struct request {
    uint64_t req_id;
    struct timespec req_timestamp;
    struct timespec req_length;
};

struct response {
    uint64_t req_id;
    uint64_t reserved;
    uint8_t  ack;
};
