TARGETS = server_mt server_q
LIBS = timelib
LDFLAGS = -lm -lpthread
BUILDDIR = build
SRC_DIR = .
INCLUDE_DIR = .
# 创建构建目录
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# 编译所有目标
all: $(BUILDDIR)/server_mt $(BUILDDIR)/server_q

# 编译server_mt
$(BUILDDIR)/server_mt: $(BUILDDIR)/server_mt.o $(BUILDDIR)/timelib.o
	gcc -o $@ $^ $(LDFLAGS) -W -Wall

# 编译server_q
$(BUILDDIR)/server_q: $(BUILDDIR)/server_q.o $(BUILDDIR)/timelib.o
	gcc -o $@ $^ $(LDFLAGS) -W -Wall 

# 编译源文件到对象文件
$(BUILDDIR)/%.o: $(SRC_DIR)/%.c | $(BUILDDIR)
	gcc -I$(INCLUDE_DIR) -c $< -o $@ -W -Wall

# 清理生成的文件
clean:
	rm -rf $(BUILDDIR) *~

