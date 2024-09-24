TARGETS = server_mt server_q
LIBS = timelib common
LDFLAGS = -lm -lpthread
BUILDDIR = build
BUILD_TARGETS = $(addprefix $(BUILDDIR)/,$(TARGETS))
OBJS = $(addprefix $(BUILDDIR)/,$(addsuffix .o,$(TARGETS) $(LIBS)))
LIBOBJS = $(addprefix $(BUILDDIR)/,$(addsuffix .o,$(LIBS)))

all: $(BUILD_TARGETS)

$(BUILD_TARGETS): $(BUILDDIR) $(OBJS)
	gcc -o $@ $(BUILDDIR)/$(notdir $@).o $(LIBOBJS) $(LDFLAGS) -W -Wall

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: %.c
	gcc -o $@ -c $< -W -Wall

clean:
	rm -f *~ $(BUILDDIR)/*.o $(BUILD_TARGETS)
