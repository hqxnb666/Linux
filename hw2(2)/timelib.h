#ifndef TIMELIB_H
#define TIMELIB_H

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

/* How many nanoseconds in a second */
#define NANO_IN_SEC (1000*1000*1000)

/* Macro wrapper for RDTSC instruction */
#define get_clocks(clocks) \
    do { \
        unsigned int __a, __d; \
        __asm__ volatile("rdtsc" : "=a" (__a), "=d" (__d)); \
        (clocks) = ((unsigned long long)__a) | (((unsigned long long)__d) << 32); \
    } while(0)

/* Return the number of clock cycles elapsed when waiting for
 * wait_time seconds using sleeping functions */
uint64_t get_elapsed_sleep(long sec, long nsec);

/* Return the number of clock cycles elapsed when waiting for
 * wait_time seconds using busy-waiting functions */
uint64_t get_elapsed_busywait(long sec, long nsec);

/* Busywait for the amount of time described via the delay
 * parameter */
uint64_t busywait_timespec(struct timespec delay);

/* Add two timespec structures together */
void timespec_add(struct timespec *a, struct timespec *b);

/* Compare two timespec structures with one another
 * Returns:
 *  -1 if a < b
 *   0 if a == b
 *   1 if a > b
 */
int timespec_cmp(struct timespec *a, struct timespec *b);

#endif // TIMELIB_H

