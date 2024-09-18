#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <time.h>

/* 定义用于打印错误信息的宏 */
#define ERROR_INFO() fprintf(stderr, "Runtime error at %s:%d\n", __FILE__, __LINE__)

/* 将timespec结构转换为double类型的宏 */
#define NANO_IN_SEC 1000000000L
#define TSPEC_TO_DOUBLE(spec) ((double)(spec.tv_sec) + (double)(spec.tv_nsec) / NANO_IN_SEC)

/* 请求结构体 */
struct request {
    uint64_t request_id;
    struct timespec sent_time;
    struct timespec req_length;
}__attribute__((packed));

/* 响应结构体 */
struct response {
    uint64_t request_id;
    uint64_t reserved;
    uint8_t ack;
}__attribute__((packed));

#endif /* COMMON_H */
