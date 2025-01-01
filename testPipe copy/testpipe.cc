#include <iostream>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include<string>
#include <unistd.h> 
#include <sys/wait.h>
#include <sys/types.h> 
const int size = 1024;
std::string getOtherMessage()
{
    static  int cnt = 0; //只在当前作用域有效，但生命周期伴随整个程序
    std::string messageid = std::to_string(cnt);
    cnt++;
    pid_t self_id = getpid();
    std::string stringpid = std::to_string(self_id);
    std::string message = " massageid: ";
    message += messageid;
    message += " string pid: ";
    message += stringpid;
    return message;

    }
void  SubProcessWrite(int wfd)
{
    std::string message = "father, I am you son process!";
    while(1)
    {
        std::string info = message + getOtherMessage();
        write(wfd,info.c_str(),info.size()); //写入管道，没有写入\0，也没有必要
        sleep(1);
    }
}

void FatherProcessRead(int rfd)
{
    char inbuffer[size];
    while(1)
    {
        ssize_t n  = read(rfd,inbuffer,sizeof(inbuffer));
        if(n > 0)
        {
            inbuffer[n] = 0;
            std::cout << "father message :"<< inbuffer << std::endl;
        }
        else if(n == 0)
        {
            std::cout << "pipe close" << std::endl;
            break;
        }
        else
        {
            std::cerr << "errno :" << errno << " :" << "errstring :" << strerror(errno) << std::endl;
            break;
        }
    }
}
int main()
{
    //1.创建管道
    int pipefd[2];
    int n = pipe(pipefd);//输出型参数,rfd,wrd
    if(n != 0)
    {
        std::cerr << "errno :" << errno << " :" << "errstring :" << strerror(errno) << std::endl;
        return -1;
    }
    std::cout << "pipefd[0]: " << pipefd[0] << "pipefd[1]" << pipefd[1] << std::endl;
    sleep(1);
    pid_t id = fork();
    //关闭不需要的文件描述符
    if(id == 0)
    {
        std::cout <<"子进程关闭不需要的fd,准备发消息了" << std::endl;
        //子进程
        sleep(1);
        close(pipefd[0]);
        SubProcessWrite(pipefd[1]);
        exit(0);
    }

    //父进程
    std::cout <<"父进程关闭不需要的fd,准备收消息了" << std::endl;
    sleep(1);
    close(pipefd[1]);
    FatherProcessRead(pipefd[0]);
    int status = 0;
    pid_t rid = waitpid(id,&status,0);
    if(rid > 0)
    {
        std::cout << "exit code: " << WEXITSTATUS(status) << std::endl;
        std::cout << "exit signal: " << WTERMSIG(status) << std::endl;
        std::cout << "subprocess wait done " << std::endl;
    }
    return 0;
}