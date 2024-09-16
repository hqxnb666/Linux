#ifndef TIMELIB_H
#define TIMELIB_H

#include <stdint.h>

#define get_clocks(__clks) \
    do { \
        unsigned int __hi, __lo; \
        __asm__ __volatile__ ("rdtsc" : "=a"(__lo), "=d"(__hi)); \
        (__clks) = ((uint64_t)__hi << 32) | __lo; \
    } while(0)

uint64_t get_elapsed_sleep(long sec, long nsec);
uint64_t get_elapsed_busywait(long sec, long nsec);

#endif 
