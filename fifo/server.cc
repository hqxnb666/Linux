#include "namedPipe.hpp"
int main()
{
    NamedPipe namedpipe (FIFO_PATH, CREATER);
    if (!namedpipe.OpenRead())
    {
        return 1;
    }
    while(1)
    {
        std::string str;
        int n = namedpipe.ReadNamedPipe(&str);
        if(n > 0)
        {
            std::cout << "read success" << std::endl;
            std::cout << "read" << str << std::endl;
        }
        else if(n == 0) //说明写端自动关闭，读端读到EOF
        {
            std::cout << "write end closed" << std::endl;
            break;
        }
        else
        {
            return 1;
        }
      
    }
    return 0;
}