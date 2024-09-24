/*******************************************************************************
* Time Functions Library (header)
*
* Description:
*     A library to handle various time-related functions and operations.
*
* Author:
*     Your Name <your.email@example.com>
*
* Affiliation:
*     Your Affiliation
*
* Creation Date:
*     Date
*
* Notes:
*     This header file contains declarations of time-related functions and macros.
*
*******************************************************************************/

#ifndef TIMELIB_H
#define TIMELIB_H

#include <time.h>
#include <stdint.h>

/* 每秒的纳秒数 */
#define NANO_IN_SEC (1000 * 1000 * 1000)

/* 将 timespec 转换为双精度浮点数，精确到微秒 */
#define TSPEC_TO_DOUBLE(spec) \
    ((double)(spec.tv_sec) + (double)(spec.tv_nsec) / NANO_IN_SEC)

/* 获取当前时钟周期数的宏 */
#if defined(__i386__)
#define get_clocks(clocks) \
    __asm__ volatile ("rdtsc" : "=A" (clocks))
#elif defined(__x86_64__)
#define get_clocks(clocks)                      \
    do {                                        \
        uint32_t lo, hi;                        \
        __asm__ volatile (                      \
            "rdtsc" : "=a" (lo), "=d" (hi));    \
        clocks = ((uint64_t)hi << 32) | lo;     \
    } while (0)
#else
#error "Unsupported architecture for get_clocks macro"
#endif

/* 函数声明 */

/* 返回使用睡眠函数等待指定时间期间经过的时钟周期数 */
uint64_t get_elapsed_sleep(long sec, long nsec);

/* 返回使用忙等待函数等待指定时间期间经过的时钟周期数 */
uint64_t get_elapsed_busywait(long sec, long nsec);

/* 根据指定的延迟时间进行忙等待 */
uint64_t busywait_timespec(struct timespec delay);

/* 将两个 timespec 结构体相加 */
void timespec_add(struct timespec *a, struct timespec *b);

/* 比较两个 timespec 结构体 */
int timespec_cmp(struct timespec *a, struct timespec *b);

#endif /* TIMELIB_H */

