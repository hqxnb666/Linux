#ifndef TIMELIB_H
#define TIMELIB_H

#include <stdint.h>
#include <time.h>

/* 定义每秒的纳秒数 */
#define NANO_IN_SEC 1000000000L

/* RDTSC指令的宏封装 */
#define get_clocks(clocks)                        \
    do {                                          \
        uint32_t __clocks_hi, __clocks_lo;        \
        __asm__ __volatile__ ("rdtsc"             \
                              : "=a" (__clocks_lo), "=d" (__clocks_hi)); \
        (clocks) = (((uint64_t)__clocks_hi) << 32) | __clocks_lo;        \
    } while(0)

/* 函数声明 */
uint64_t get_elapsed_sleep(long sec, long nsec);
uint64_t get_elapsed_busywait(long sec, long nsec);
void timespec_add(struct timespec* a, struct timespec* b);
int timespec_cmp(struct timespec* a, struct timespec* b);

#endif /* TIMELIB_H */
