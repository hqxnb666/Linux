#pragma once
#include <iostream>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <string>
#include <vector>

#define TaskNum 3
typedef void(*task_t)(); //task_t is a function pointer type
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

task_t task[TaskNum] ;
void Loadtask()
{
    task[0] = task1;
    task[1] = task2;
    task[2] = task3;
}