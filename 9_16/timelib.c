#include "timelib.h"
#include <time.h>

uint64_t get_elapsed_sleep(long sec, long nsec) {
    struct timespec req = { .tv_sec = sec, .tv_nsec = nsec };
    uint64_t before, after;

    get_clocks(before);
    nanosleep(&req, NULL);
    get_clocks(after);

    return after - before;
}

#include <stdio.h>
#include <stdint.h>

uint64_t get_elapsed_busywait(long sec, long nsec) {
    struct timespec begin_timestamp, current_timestamp, wait_time;

    
    wait_time.tv_sec = sec;
    wait_time.tv_nsec = nsec;


    clock_gettime(CLOCK_MONOTONIC, &begin_timestamp);

  
    do {
        clock_gettime(CLOCK_MONOTONIC, &current_timestamp);

        
        if ((current_timestamp.tv_sec > begin_timestamp.tv_sec) ||
            (current_timestamp.tv_sec == begin_timestamp.tv_sec && current_timestamp.tv_nsec >= begin_timestamp.tv_nsec + wait_time.tv_nsec)) {
            break;
        }
    } while (1);

    struct timespec elapsed_time;
    elapsed_time.tv_sec = current_timestamp.tv_sec - begin_timestamp.tv_sec;
    elapsed_time.tv_nsec = current_timestamp.tv_nsec - begin_timestamp.tv_nsec;

  
    uint64_t elapsed_ns = elapsed_time.tv_sec * 1000000000L + elapsed_time.tv_nsec;

    return elapsed_ns;
}

