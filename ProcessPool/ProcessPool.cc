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
    
    void Wait()
    {
        int status;
        if (waitpid(_subprocesspid, &status, 0) == -1)
        {
            std::cerr << "waitpid failed: " << strerror(errno) << std::endl;
        }
        if(waitpid(_subprocesspid, &status, 0) > 0)
        {
            std::cout << "waitpid success" << std::endl;
        }
        if (WIFEXITED(status))
        {
            std::cout << _name << " exited with status " << WEXITSTATUS(status) << std::endl;
        }
        else
        {
            std::cerr << _name << " exited abnormally" << std::endl;
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
 void CreateChannelsandSubprocesses(int subprocessnum, std::vector<Channel> *channels, task_t task)
    {
        for (int i = 0; i < subprocessnum; i++)
        {
            int pipefd[2];
            if (pipe(pipefd) == -1)
            {
                std::cerr << "pipe failed: " << strerror(errno) << std::endl;
                exit(1);
            }
            pid_t id = fork();
            if (id == -1)
            {
                std::cerr << "fork failed:" << strerror(errno) << std::endl;
            }
            if (id == 0)
            {
                if (i != 0)
                {
                    // 说明当前是第二个子进程
                    for (int j = 0; j < i; j++)
                    {
                        close((*channels)[j].getWfd());
                    }
                    
                }
                close(pipefd[1]);
                dup2(pipefd[0], STDIN_FILENO);
                    task();
                    close(pipefd[0]);
                    exit(0);

            }
            close(pipefd[0]);
            channels->push_back(Channel(pipefd[1], id, "subprocess" + std::to_string(i)));

        }
    }
    int NextChannelIndex(int size)
    {
        static int index = 0;
        return index++ % size;
    }
    void SendTask(int wfd, int tasknum)
    {
        if (write(wfd, &tasknum, sizeof(tasknum)) == -1)
        {
            std::cerr << "write failed: " << strerror(errno) << std::endl;
        }

    }
    void controlProcessonce(std::vector<Channel>& channels)
    {
        int tasknum = Selecttask();
        int channel_index = NextChannelIndex(channels.size());
        SendTask(channels[channel_index].getWfd(), tasknum);

    }
    void controlProcess(std::vector<Channel>& channels,int times = -1)
    
    {
        if(times > 0)
        {
            while(times--)
            {
                controlProcessonce(channels);
            }
        }
        else
        {
            while(1)
            {
                controlProcessonce(channels);
            }
        }
    }
     void cleanupchannels(std::vector<Channel>& channels)
     {
        for(auto& channel:channels)
        {
            channel.CloseChannel();
            channel.Wait();
        }
     }
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
    CreateChannelsandSubprocesses(subprocessnum, &channels, work1);
    // 2. control the subprocesses
    controlProcess(channels, 10);
    // 3. close the channels
    cleanupchannels(channels);
    return 0;
}