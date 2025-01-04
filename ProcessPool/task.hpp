#pragma once
#include <iostream>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <string>
#include <vector>

#define TaskNum 3
typedef void (*task_t)(); // task_t is a function pointer type
task_t task[TaskNum];
void task1()
{
    std::cout << "task1" << std::endl;
}
void task2()
{
    std::cout << "task2" << std::endl;
}
void task3()
{
    std::cout << "task3" << std::endl;
}
void ExcuteTask(int count)
{
    if (count >= 0 && count < TaskNum)
    {
        task[count]();
    }
    else
    {
        std::cerr << "task index out of range" << std::endl;
        return;
    }
}
void work()
{
    while (1)
    {
        int count = 0;
        int n = read(0, &count, sizeof(count));
        if (n == -1)
        {
            std::cerr << "read failed: " << strerror(errno) << std::endl;
            exit(1);
        }
        if (n == sizeof(count))
        {
            std::cout << "pid is : " << getpid() << " handler task" << std::endl;
            ExcuteTask(count);
        }
        if (n == 0)
        {
            std::cout << "read EOF" << std::endl;
            exit(0);
        }
    }
}
void work1()
{
    while (1)
    {
        int count = 1;
        int n = read(0, &count, sizeof(count));
        if (n == -1)
        {
            std::cerr << "read failed: " << strerror(errno) << std::endl;
            exit(1);
        }
        if (n == sizeof(count))
        {
            std::cout << "pid is : " << getpid() << " handler task" << std::endl;
            ExcuteTask(count);
        }
        if (n == 0)
        {
            std::cout << "read EOF" << std::endl;
            exit(0);
        }
    }
}
void work2()
{
    while (1)
    {
        int count = 2;
        int n = read(0, &count, sizeof(count));
        if (n == -1)
        {
            std::cerr << "read failed: " << strerror(errno) << std::endl;
            exit(1);
        }
        if (n == sizeof(count))
        {
            std::cout << "pid is : " << getpid() << " handler task" << std::endl;
            ExcuteTask(count);
        }
        if (n == 0)
        {
            std::cout << "read EOF" << std::endl;
            exit(0);
        }
    }
}  
int Selecttask()
    {
        return rand() % TaskNum;
    }

void Loadtask()
{
    task[0] = task1;
    task[1] = task2;
    task[2] = task3;
}