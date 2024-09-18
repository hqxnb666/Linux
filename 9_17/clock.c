#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "timelib.h"

int main(int argc, char** argv)
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <seconds> <nanoseconds> <s|b>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    uint64_t elapsed_cycles;
    long sec = atol(argv[1]);
    long nsec = atol(argv[2]);
    char method = argv[3][0];

    if (method == 's') {
        elapsed_cycles = get_elapsed_sleep(sec, nsec);
        printf("WaitMethod: SLEEP\n");
    }
    else if (method == 'b') {
        elapsed_cycles = get_elapsed_busywait(sec, nsec);
        printf("WaitMethod: BUSYWAIT\n");
    }
    else {
        fprintf(stderr, "Invalid method. Use 's' for sleep or 'b' for busywait.\n");
        exit(EXIT_FAILURE);
    }

    printf("WaitTime: %ld %ld\n", sec, nsec);
    printf("ClocksElapsed: %lu\n", elapsed_cycles);

    double time_in_sec = sec + (double)nsec / 1e9;
    double clock_speed_mhz = elapsed_cycles / (time_in_sec * 1e6);

    printf("ClockSpeed: %.6f\n", clock_speed_mhz);

    return EXIT_SUCCESS;
}
