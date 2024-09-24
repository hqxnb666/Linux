###############################################################################
# Makefile for Compiling Clock, TimeLib, Server, and Client Modules
#
# Description:
#     This Makefile is designed to compile various components, including:
#     - CPU Clock measurement functions using RDTSC
#     - TimeLib: A library for time-related operations
#     - FIFO Order Server: Processes client requests in FIFO order
#     - FIFO Order Client: Sends requests to the server
#
# Targets:
#     - all: Compiles all modules
#     - clock: Compiles the CPU clock measurement functions
#     - server: Compiles the server executable
#     - client: Compiles the client executable
#     - clean: Removes compiled binaries and intermediate files
#
# Usage:
#     make <target_name>
#     NOTE: all the binaries will be created in the build/ subfolder
#
# Author:
#     Renato Mancuso
#
# Affiliation:
#     Boston University
#
# Creation Date:
#     September 10, 2023
#
# Last Changes:
#     September 16, 2024
#
#
# Notes:
#     Ensure you have the necessary dependencies and permissions before
#     compiling. Modify the Makefile as necessary if directory structures
#     change or if additional modules are added in the future.
#
###############################################################################


TARGETS = server_mt server_q
LIBS = timelib
LDFLAGS = -lm -lpthread
BUILDDIR = build
BUILD_TARGETS = $(addprefix $(BUILDDIR)/,$(TARGETS))
OBJS = $(addprefix $(BUILDDIR)/,$(addsuffix .o,$(TARGETS) $(LIBS)))
LIBOBJS = $(addprefix $(BUILDDIR)/,$(addsuffix .o,$(LIBS)))

all: $(BUILD_TARGETS)

$(BUILD_TARGETS): $(BUILDDIR) $(OBJS)
	gcc -o $@ $@.o $(LIBOBJS) $(LDFLAGS) -W -Wall

$(BUILDDIR):
	mkdir $(BUILDDIR)

$(BUILDDIR)/%.o: %.c
	gcc -o $@ -c $< -W -Wall

clean:
	rm *~ -rf $(BUILDDIR)

