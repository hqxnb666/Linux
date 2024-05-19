#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main()
{
  pid_t id = fork();
  if(id == 0)
  {
    //child
    int cnt = 5;
    while(cnt)
    {
      printf("I am child, cnt:%d, pid:%d\n", cnt, getpid());
      sleep(1);
      cnt--;
    }
  }
  else 
  {
    //parent
    while(1)
    {
      printf("I am parent, pid:%d\n",getpid());
      sleep(1);
    }
  }
  return 0;
}
