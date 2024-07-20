#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
void ChildRun()
{
  int cnt = 5;
  while(cnt)
  {
    printf("I am child process, pid : %d, ppid : %d, cnt:%d\n",getpid(),getppid(),cnt);
    sleep(1);
    cnt--;
  }
}

int main()
{

    printf("I am father, pid : %d, ppid:%d\n",getpid(),getppid());
    pid_t id = fork();
    if(id == 0)
    {
      ChildRun();
      printf("child quit..\n");
      exit(0);
    }
    sleep(5);
    int status = 0;
    pid_t rid = waitpid(id,&status,0);
    if(rid >0){
      printf("wait success,rid:%d\n",rid);
    }
    else{
      printf("wait failed!\n");
    }
    sleep(3);
    printf("father quit, status: %d, child quit code: %d, child quit signal: %d\n",status,(status >> 8)&0xFF, status&0x7F);
}


