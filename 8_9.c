#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main()
{
  pid_t pid = fork();
  if(pid ==0)
  {
    char* args[] = {"ls", "-l", "-a",NULL};
    execvp(args[0],args);
    return 1;;
  }else if(pid > 0 )
  {
    wait(NULL);
  }else{
    perror("fork");
    return 1;
  }
  return 0;
}
