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
      exit(123);
    }
    //father
    while(1)
    {
      int status = 0;
      pid_t rid = waitpid(id,&status,WNOHANG);
      if(rid == 0)
      {
        usleep(1000);
        printf("child is running, father check next time\n");

      }
      else if(rid >0)
      {
        if(WIFEXITED(status))
        {
          printf("child quit success, child exit code: %d\n",WEXITSTATUS(status));
        }else{
          if(WIFSIGNALED(status))
          {
            printf("child was killed by signedl,quit signal :%d\n",WTERMSIG(status));
          }
        }
        break;
      }
      else{
        printf("waitpid failed\n");
        break;
      }
    }
   // sleep(5);
    
    //pid_t rid = waitpid(id,&status,0);
    //if(rid >0){
    //  printf("wait success,rid:%d\n",rid);
    //}
   // else{
   //   printf("wait failed!\n");
   // }
   // sleep(3);
   // printf("father quit, status: %d, child quit code: %d, child quit signal: %d\n",status,(status >> 8)&0xFF, status&0x7F);
}


