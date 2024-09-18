#include "timelib.h"
#include <time.h>
#include <stdint.h>
#include <unistd.h>

/* 使用sleep测量经过的时钟周期 */
uint64_t get_elapsed_sleep(long sec, long nsec)
{
    uint64_t before, after;
    struct timespec req = { sec, nsec };

    get_clocks(before);
    nanosleep(&req, NULL);
    get_clocks(after);

    return after - before;
}

/* 使用忙等待测量经过的时钟周期 */
uint64_t get_elapsed_busywait(long sec, long nsec)
{
    struct timespec start_time, current_time;
    uint64_t before, after;

    clock_gettime(CLOCK_MONOTONIC, &start_time);
    get_clocks(before);

    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &current_time);

        long elapsed_sec = current_time.tv_sec - start_time.tv_sec;
        long elapsed_nsec = current_time.tv_nsec - start_time.tv_nsec;

        if (elapsed_nsec < 0) {
            elapsed_sec -= 1;
            elapsed_nsec += NANO_IN_SEC;
        }

        if (elapsed_sec > sec || (elapsed_sec == sec && elapsed_nsec >= nsec)) {
            break;
        }
    }

    get_clocks(after);

    return after - before;
}

/* 添加两个timespec结构体 */
void timespec_add(struct timespec* a, struct timespec* b)
{
    a->tv_sec += b->tv_sec;
    a->tv_nsec += b->tv_nsec;

    if (a->tv_nsec >= NANO_IN_SEC) {
        a->tv_sec += 1;
        a->tv_nsec -= NANO_IN_SEC;
    }
}

/* 比较两个timespec结构体 */
int timespec_cmp(struct timespec* a, struct timespec* b)
{
    if (a->tv_sec > b->tv_sec) {
        return 1;
    }
    else if (a->tv_sec < b->tv_sec) {
        return -1;
    }
    else {
        if (a->tv_nsec > b->tv_nsec) {
            return 1;
        }
        else if (a->tv_nsec < b->tv_nsec) {
            return -1;
        }
        else {
            return 0;
        }
    }
}
