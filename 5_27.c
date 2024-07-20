#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main()

{
  pid_t id = fork();
  if(id == 0)
  {
    while(1)
    {
      printf("I am child, pid = %d, ppid = %d\n",getpid(),getppid()); 
      sleep(2);
    }
  }else 
  {
    
    while(1)
    {
      printf("I am father, pid = %d, ppid = %d\n",getpid(),getppid()); 
      sleep(2);
  }
  }
  return 0;
}
