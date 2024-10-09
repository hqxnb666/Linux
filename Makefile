###############################################################################
# Makefile for Compiling TimeLib, Server, and Client Modules
#
# Description:
#     This Makefile is designed to compile various components, including:
#     - TimeLib: A library for time-related operations
#     - FIFO Order Server: Processes client requests in FIFO order
#     - FIFO Order Client: Sends requests to the server
#
# Targets:
#     - all: Compiles all modules
#     - server_lim: Compiles the server w/ limited queue executable
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
#     October 22, 2023
#
# Last Update:
#     September 25, 2024
#
#
# Notes:
#     Ensure you have the necessary dependencies and permissions before
#     compiling. Modify the Makefile as necessary if directory structures
#     change or if additional modules are added in the future.
#
###############################################################################


TARGETS = server_multi
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
