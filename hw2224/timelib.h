/*******************************************************************************
* Time Functions Library (header)
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

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

/* How many nanoseconds in a second */
#define NANO_IN_SEC (1000*1000*1000)

/* Macro wrapper for RDTSC instruction */
#ifdef __x86_64__
#define get_clocks(clocks) \
    do { \
        unsigned int __a, __d; \
        asm volatile("rdtsc" : "=a" (__a), "=d" (__d)); \
        (clocks) = ((uint64_t)__d << 32) | __a; \
    } while(0)
#else
#error "get_clocks is only implemented for x86_64 architecture."
#endif
   
/* IMPLEMENT ME! See lecture slides for inspiration. */

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
void timespec_add (struct timespec *, struct timespec *);

/* Compare two timespec structures with one another */
int timespec_cmp(struct timespec *, struct timespec *);
