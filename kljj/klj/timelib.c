#include "timelib.h"

uint64_t get_elapsed_sleep(long sec, long nsec)
{
    uint64_t begin, finish;
    struct timespec sleep_time;
    sleep_time.tv_sec = sec;
    sleep_time.tv_nsec = nsec;

    get_clocks(begin);

    nanosleep(&sleep_time, NULL);

    get_clocks(finish);

    return (finish - begin);
}

uint64_t get_elapsed_busywait(long sec, long nsec)
{
    uint64_t begin, finish;
    struct timespec current_time;
    struct timespec end_time;

    clock_gettime(CLOCK_MONOTONIC, &current_time);
    end_time.tv_sec = sec;
    end_time.tv_nsec = nsec;
    timespec_add(&end_time, &current_time);

    get_clocks(begin);

    do {
        clock_gettime(CLOCK_MONOTONIC, &current_time);
    } while (end_time.tv_sec > current_time.tv_sec || end_time.tv_nsec > current_time.tv_nsec);

    get_clocks(finish);

    return (finish - begin);
}

void timespec_add(struct timespec *a, struct timespec *b)
{
    time_t carry_seconds = b->tv_sec;
    a->tv_nsec += b->tv_nsec;
    if (a->tv_nsec > NANO_IN_SEC) {
        carry_seconds += a->tv_nsec / NANO_IN_SEC;
        a->tv_nsec = a->tv_nsec % NANO_IN_SEC;
    }
    a->tv_sec += carry_seconds;
}

int timespec_cmp(struct timespec *a, struct timespec *b)
{
    if (a->tv_sec == b->tv_sec && a->tv_nsec == b->tv_nsec) {
        return 0;
    } else if ((a->tv_sec > b->tv_sec) ||
               (a->tv_sec == b->tv_sec && a->tv_nsec > b->tv_nsec)) {
        return 1;
    } else {
        return -1;
    }
}

uint64_t busywait_timespec(struct timespec delay)
{
    uint64_t begin, finish;
    struct timespec current_time;

    clock_gettime(CLOCK_MONOTONIC, &current_time);
    timespec_add(&delay, &current_time);

    get_clocks(begin);

    do {
        clock_gettime(CLOCK_MONOTONIC, &current_time);
    } while (delay.tv_sec > current_time.tv_sec || delay.tv_nsec > current_time.tv_nsec);

    get_clocks(finish);

    return (finish - begin);
}

inline struct timespec dtotspec(double timestamp)
{
    struct timespec result;
    result.tv_sec = (long)timestamp;
    result.tv_nsec = (long)(timestamp * NANO_IN_SEC) % NANO_IN_SEC;
    return result;
}
