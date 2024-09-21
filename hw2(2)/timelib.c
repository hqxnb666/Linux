#include "timelib.h"
#include <errno.h> // 添加此行

/* Return the number of clock cycles elapsed when waiting for
 * wait_time seconds using sleeping functions */
uint64_t get_elapsed_sleep(long sec, long nsec) {
    uint64_t start, end;
    get_clocks(start);
    struct timespec req, rem;
    req.tv_sec = sec;
    req.tv_nsec = nsec;
    while (nanosleep(&req, &rem) == -1 && errno == EINTR) {
        req = rem;
    }
    get_clocks(end);
    return end - start;
}

/* Return the number of clock cycles elapsed when waiting for
 * wait_time seconds using busy-waiting functions */
uint64_t get_elapsed_busywait(long sec, long nsec) {
    uint64_t start, end;
    get_clocks(start);
    struct timespec delay;
    delay.tv_sec = sec;
    delay.tv_nsec = nsec;
    busywait_timespec(delay);
    get_clocks(end);
    return end - start;
}

/* Busywait for the amount of time described via the delay
 * parameter */
uint64_t busywait_timespec(struct timespec delay) {
    struct timespec start, current;
    clock_gettime(CLOCK_MONOTONIC, &start);
    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &current);
        // Calculate elapsed time
        long sec = current.tv_sec - start.tv_sec;
        long nsec = current.tv_nsec - start.tv_nsec;
        if (nsec < 0) {
            sec -= 1;
            nsec += NANO_IN_SEC;
        }
        if (sec > delay.tv_sec || (sec == delay.tv_sec && nsec >= delay.tv_nsec)) {
            break;
        }
    }
    return 0;
}

/* Add two timespec structures together */
void timespec_add(struct timespec *a, struct timespec *b) {
    a->tv_sec += b->tv_sec;
    a->tv_nsec += b->tv_nsec;
    if (a->tv_nsec >= NANO_IN_SEC) {
        a->tv_sec += 1;
        a->tv_nsec -= NANO_IN_SEC;
    }
}

/* Compare two timespec structures with one another
 * Returns:
 *  -1 if a < b
 *   0 if a == b
 *   1 if a > b
 */
int timespec_cmp(struct timespec *a, struct timespec *b) {
    if (a->tv_sec < b->tv_sec)
        return -1;
    if (a->tv_sec > b->tv_sec)
        return 1;
    if (a->tv_nsec < b->tv_nsec)
        return -1;
    if (a->tv_nsec > b->tv_nsec)
        return 1;
    return 0;
}

