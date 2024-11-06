/*******************************************************************************
* Performance Hardware Counter Library (header)
*
* Description:
*     A library to handle interacting with hardware counters.
*
* Author:
*     Anna Arpaci-Dusseau (annaad@bu.edu)
*
* Affiliation:
*     Boston University
*
* Creation Date:
*     October 31, 2024
*
* Notes:
*     Ensure to link against the necessary dependencies when compiling and
*     using this library. Modifications or improvements are welcome. Please
*     refer to the accompanying documentation for detailed usage instructions.
*
*******************************************************************************/

#include <linux/perf_event.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>

/* Internally used wrapper around syscall to open performance counter.
 * Returns a file descriptor on success, returns -1 on failure. 
*/
long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                int cpu, int group_fd, unsigned long flags);

/* Sets up a performance counter for specified hardware event
 * based on a <type> and <config> value. Returns a file descriptor (handle)
 * corresponding to this specific performance counter.
 * NOTE: this must be called from the thread you wish to monitor (i.e. the worker) 
*/
int setup_perf_counter(uint64_t type, uint64_t config);


/* Read the current value of the performance counter specified by a file descriptor.
 * NOTE: you must use a call to ioctl() to RESET/ENABLE the performance counter prior to reading.
*/
uint64_t read_perf_counter(int fd);
