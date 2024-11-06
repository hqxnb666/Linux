/*******************************************************************************
* Performance Hardware Counter Library (implementation)
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


#include "perflib.h"


/* Internally used wrapper around syscall to open performance counter file descriptor */
long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
		     int cpu, int group_fd, unsigned long flags) {
	int ret;

	ret = syscall(SYS_perf_event_open, hw_event, pid, cpu,
		      group_fd, flags);
	return ret;
}

/* Sets up a performance counter for specified hardware event
 * based on a <type> and <config> value. Returns a file descriptor (handle)
 * corresponding to this specific performance counter. 
 * NOTE: this must be called from the thread you wish to monitor (i.e. the worker)
*/
int setup_perf_counter(uint64_t type, uint64_t config) {
	int                     fd;
	struct perf_event_attr  pe;

	// configure the perf_event_attr structure based on type, config, and no kernel monitoring
	memset(&pe, 0, sizeof(pe));
	pe.type = type;
	pe.size = sizeof(pe);
	pe.config = config;
	pe.disabled = 1;
	pe.exclude_kernel = 1;
	pe.exclude_hv = 1;

	// we always set pid = 0, and cpu = -1. This means track this process, on any cpu.
	fd = perf_event_open(&pe, 0, -1, -1, 0);
	if (fd == -1) {
		fprintf(stderr, "Error opening leader %llx\n", pe.config);
		exit(EXIT_FAILURE);
	}

	return fd;

}

/* Read the current value of the performance counter specified by a file descriptor.
 * NOTE: you must use a call to ioctl() to RESET/ENABLE the performance counter prior to reading.
*/
uint64_t read_perf_counter(int fd) {
	uint64_t count = 0xbeefcafeUL;
	read(fd, &count, sizeof(count));
	return count;
}
