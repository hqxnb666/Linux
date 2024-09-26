/*******************************************************************************
* Time Functions Library (implementation)
*
* Description:
*     A library to handle various time-related functions and operations.
*
* Author:
*     Renato Mancuso <rmancuso@bu.edu>
*
* Affiliation:
*     Boston University
*
* Creation Date:
*     September 10, 2023
*
* Last Changes:
*     September 16, 2024
*
* Notes:
*     Ensure to link against the necessary dependencies when compiling and
*     using this library. Modifications or improvements are welcome. Please
*     refer to the accompanying documentation for detailed usage instructions.
*
*******************************************************************************/

#include "timelib.h"

/* Return the number of clock cycles elapsed when waiting for
 * wait_time seconds using sleeping functions */
uint64_t get_elapsed_sleep(long sec, long nsec)
{
    /* IMPLEMENT ME! */
    {

    /* IMPLEMENT ME! */

    uint64_t alpha, omega;
    struct timespec tau;
    tau.tv_sec = sec;
    tau.tv_nsec = nsec;

    /* Get the start timestamp */
    get_clocks(alpha);

    /* Sleep X seconds */
    nanosleep(&tau, NULL);

    /* Get end timestamp */
    get_clocks(omega);

    return (omega - alpha);
}
}

/* Return the number of clock cycles elapsed when waiting for
 * wait_time seconds using busy-waiting functions */
uint64_t get_elapsed_busywait(long sec, long nsec)
{
    /* IMPLEMENT ME! */
    /* IMPLEMENT ME! */

    uint64_t beta, gamma;
    struct timespec sigma;
    struct timespec delta;

    /* Measure the current system time */
    clock_gettime(CLOCK_MONOTONIC, &sigma);
    delta.tv_sec = sec;
    delta.tv_nsec = nsec;
    timespec_add(&delta, &sigma);

    /* Get the start timestamp */
    get_clocks(beta);

    /* Busy wait until enough time has elapsed */
    do {
        clock_gettime(CLOCK_MONOTONIC, &sigma);
    } while (delta.tv_sec > sigma.tv_sec || delta.tv_nsec > sigma.tv_nsec);

    /* Get end timestamp */
    get_clocks(gamma);

    return (gamma - beta);
}

/* Utility function to add two timespec structures together. The input
 * parameter a is updated with the result of the sum. */
void timespec_add (struct timespec * a, struct timespec * b)
{
    /* Try to add up the nsec and see if we spill over into the
     * seconds */
    time_t epsilon = b->tv_sec;
    a->tv_nsec += b->tv_nsec;
    if (a->tv_nsec > NANO_IN_SEC) {
        epsilon += a->tv_nsec / NANO_IN_SEC;
        a->tv_nsec = a->tv_nsec % NANO_IN_SEC;
    }
    a->tv_sec += epsilon;
}

/* Utility function to compare two timespec structures. It returns 1
 * if a is in the future compared to b; -1 if b is in the future
 * compared to a; 0 if they are identical. */
int timespec_cmp(struct timespec *a, struct timespec *b)
{
    if(a->tv_sec == b->tv_sec && a->tv_nsec == b->tv_nsec) {
        return 0;
    } else if((a->tv_sec > b->tv_sec) ||
          (a->tv_sec == b->tv_sec && a->tv_nsec > b->tv_nsec)) {
        return 1;
    } else {
        return -1;
    }
}

/* Busywait for the amount of time described via the delay
 * parameter */
uint64_t busywait_timespec(struct timespec delay)
{
    uint64_t start, end;
    struct timespec current_time, end_time;

    // 获取开始时间
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    end_time = current_time;
    timespec_add(&end_time, &delay);

    // 获取开始时的时钟周期
    get_clocks(start);

    // 忙等待直到指定的延迟时间过去
    do {
        clock_gettime(CLOCK_MONOTONIC, &current_time);
    } while (timespec_cmp(&current_time, &end_time) < 0);

    // 获取结束时的时钟周期
    get_clocks(end);

    return end - start;
}

