# 定义编译器和编译选项
CC = gcc
CFLAGS = -Wall -g

# 定义目标可执行文件和源文件
TARGETS = server_mt server_q
SRCS_COMMON = timelib.c

# 默认目标
all: $(TARGETS)

# 编译 server_mt
server_mt: server_mt.c $(SRCS_COMMON)
	$(CC) $(CFLAGS) -o $@ $^ -lpthread

# 编译 server_q
server_q: server_q.c $(SRCS_COMMON)
	$(CC) $(CFLAGS) -o $@ $^ -lpthread

# 清理
clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean
