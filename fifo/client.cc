#include "namedPipe.hpp"
int main()
{
    NamedPipe namedPipe(FIFO_PATH, USER);
    if (!namedPipe.OpenWrite())
    {
        return 1;
    }
   
    while (1)
    {
        std::string str;
        std::cin >> str;
        if (str == "exit")
        {
            break;
        }
        if (!namedPipe.WriteNamedPipe(str))
        {
            return 1;
        }
    }
    return 0;
}