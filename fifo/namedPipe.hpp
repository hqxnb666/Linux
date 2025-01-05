#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
const std::string FIFO_PATH = "./my_fifo";
#define CREATER 1
#define USER 2
#define DEFAULT -1
#define READ O_RDONLY
#define WRITE O_WRONLY
#define BASESIZE 1024

class NamedPipe
{
    private:
    bool OpenNamedPipe(int mode)
    {
        if(open(_fifo_path.c_str(), mode) == -1)
        {
            std::cerr << "open failed: " << strerror(errno) << std::endl;
            return false;
        }
        return true;
    }
public:
    NamedPipe(const std::string &fifo_path, int id)
        : _fifo_path(fifo_path), _id(id), _fd(-1)
    {
       if(_id == CREATER)
       {
        if(mkfifo(_fifo_path.c_str(), 0666) == -1)
        {
            std::cerr << "mkfifo failed: " << strerror(errno) << std::endl;
            exit(1);
        }
        std::cout << "mkfifo success" << std::endl;
       }
    }
    ~NamedPipe()
    {
        if(_id == CREATER)
        {
            if(unlink(_fifo_path.c_str()) == -1)
            {
                std::cerr << "unlink failed: " << strerror(errno) << std::endl;
            }
            std::cout << "unlink success" << std::endl;
        }
        if(_fd != DEFAULT)
        {
            if(close(_fd) == -1)
            {
                std::cerr << "close failed: " << strerror(errno) << std::endl;
            }
            std::cout << "close success" << std::endl;
        }
    }
    bool OpenRead()
    {
        _fd = open(_fifo_path.c_str(), READ);
        if(_fd == -1)
        {
            std::cerr << "open failed: " << strerror(errno) << std::endl;
            return false;
        }
        return true;
    }
    bool OpenWrite()
    {
        if((_fd = open(_fifo_path.c_str(), WRITE)) == -1)
        {
            std::cerr << "open failed: " << strerror(errno) << std::endl;
        }
        return true;
    }
    int ReadNamedPipe(std::string* buf)
    {
        char buffer[BASESIZE];
        int n = read(_fd, buffer, BASESIZE);
        if(n == -1)
        {
            std::cerr << "read failed: " << strerror(errno) << std::endl;
            return -1;
        }
        *buf = buffer;
        return n;
    }
    int WriteNamedPipe(const std::string &buf)
    {
        int n = write(_fd, buf.c_str(), buf.size());
        if(n == -1)
        {
            std::cerr << "write failed: " << strerror(errno) << std::endl;
            return -1;
        }
        return n;
    }
    private:
    const std::string _fifo_path;
    int _fd;
    int _id;
};