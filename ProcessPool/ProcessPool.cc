#include <iostream>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include "task.hpp"
class Channel
{
public:
    Channel(int wfd, pid_t subprocesspid, std::string name)
        : _wfd(wfd), _subprocesspid(subprocesspid), _name(name)
    {
    }
    int getWfd() const
    {
        return _wfd;
    }
    pid_t getSubprocesspid() const
    {
        return _subprocesspid;
    }
    std::string getName() const
    {
        return _name;
    }
    // 形参类型和命名规范
    // const &: 输出
    // & : 输入输出型参数
    // * : 输出型参数
    //  task_t task: 回调函数
    void CreateChannelsandSubprocesses(int subprocessnum, std::vector<Channel>* channels,task_t task)
    {
        for(int i  = 0; i< subprocessnum; i++)
        {
            int pipefd[2];
            if(pipe(pipefd) == -1)
            {
                std::cerr << "pipe failed: " << strerror(errno) << std::endl;
                exit(1);
            }
            pid_t id = fork();
            if(id == -1)
            {
                std::cerr <<"fork failed:" << strerror(errno) << std::endl;
            }
            if(id == 0 )
            {
                if(i!= 0)
                {
                    //说明当前是第二个子进程，要
                }
            }
        }
    }

    void CloseChannel()
    {
        if (close(_wfd) == -1)
        {
            std::cerr << "close " << _name << " failed: " << strerror(errno) << std::endl;
        }
    }

private:
    int _wfd;
    pid_t _subprocesspid;
    std::string _name;
};
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage:" << argv[0] << "subprocessnum" << std::endl;
    }
    int subprocessnum = atoi(argv[1]);
    if (subprocessnum <= 0)
    {
        std::cerr << "subprocessnum must be greater than 0" << std::endl;
    }
    Loadtask();

    std::vector<Channel> channels;
    // 1. create subprocessnum subprocesses and establish communication channels
    CreateChannelsandSubprocesses(subprocessnum, channels);
    return 0;
}