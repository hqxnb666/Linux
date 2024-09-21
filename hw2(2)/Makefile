# Makefile for Building Multi-threaded FIFO Server Project

# 编译器和编译选项
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=gnu99
LDFLAGS = -lpthread -lrt

# 头文件
HEADERS = common.h timelib.h

# 源文件
SERVER_MT_SRC = server_mt.c timelib.c
SERVER_Q_SRC = server_q.c timelib.c
CLIENT_SRC = client.c

# 可执行文件
EXECUTABLES = server_mt server_q client

# 默认目标
all: $(EXECUTABLES)

# 编译 server_mt
server_mt: $(SERVER_MT_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o server_mt $(SERVER_MT_SRC) $(LDFLAGS)

# 编译 server_q
server_q: $(SERVER_Q_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o server_q $(SERVER_Q_SRC) $(LDFLAGS)

# 编译 client
client: $(CLIENT_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o client $(CLIENT_SRC) $(LDFLAGS)

# 清理编译生成的文件
clean:
	rm -f $(EXECUTABLES) *.o

# 声明伪目标
.PHONY: all clean

