#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "timelib.h"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <seconds> <nanoseconds> <'s' or 'b'>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    long sec = atol(argv[1]);
    long nsec = atol(argv[2]);
    char *method_str = argv[3];
    uint64_t elapsed_clocks = 0;
    double clock_speed_mhz = 0.0;
    char *wait_method;

    if (strcmp(method_str, "s") == 0) {
        elapsed_clocks = get_elapsed_sleep(sec, nsec);
        wait_method = "SLEEP";
    } else if (strcmp(method_str, "b") == 0) {
        elapsed_clocks = get_elapsed_busywait(sec, nsec);
        wait_method = "BUSYWAIT";
    } else {
        fprintf(stderr, "Invalid method. Use 's' for sleep or 'b' for busywait.\n");
        exit(EXIT_FAILURE);
    }

    double wait_time_seconds = (double)sec + (double)nsec / 1e9;
    clock_speed_mhz = (double)elapsed_clocks / wait_time_seconds / 1e6;

    printf("WaitMethod: %s\n", wait_method);
    printf("WaitTime: %ld %ld\n", sec, nsec);
    printf("ClocksElapsed: %lu\n", elapsed_clocks);
    printf("ClockSpeed: %.2f\n", clock_speed_mhz);

    return 0;
}

