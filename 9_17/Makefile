# Makefile

CC = gcc
CFLAGS = -Wall -O2 
LDFLAGS =

# 目标文件
OBJS = timelib.o

# 默认目标
all: server client

# 编译 timelib.o
timelib.o: timelib.c timelib.h
	$(CC) $(CFLAGS) -c timelib.c

# 编译服务器程序
server: server.c $(OBJS) common.h
	$(CC) $(CFLAGS) -o server server.c $(OBJS) $(LDFLAGS)

# 编译客户端程序
client: client.c $(OBJS) common.h
	$(CC) $(CFLAGS) -o client client.c $(OBJS) $(LDFLAGS)

# 清理生成的文件
clean:
	rm -f *.o server client

.PHONY: all clean

