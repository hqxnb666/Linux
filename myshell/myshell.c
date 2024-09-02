#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#define SIZE 512
#define ZERO '\0'
#define SEP " "
#define NUM 32
#define SkipPath(p)do{ \
 p += (strlen(p)-1); \
 while(*p !='/'){p--;} \
}while(0)
extern int putenv(char *string);

char cwd[SIZE*2];
  const char* getusername()
  {
    const char* name = getenv("USER");
    if(name == NULL) return "NONE";
    return name;
  }
const char* Gethostname()
{
  const char* hostname = getenv("HOSTNAME");
  if(hostname == NULL) return "NONE";
  return hostname;
}

const char* getpwdname()
{
  const char* name = getenv("PWD");
  if(name == NULL){
    return "NONE";

  }
  return name;
}

void makecommandlineandPrint(char line[], size_t size)
{
  const char* username = getusername();
  const char* hostname = Gethostname();
  const char* cwdname  = getpwdname();
  SkipPath(cwdname);
  snprintf(line,size,"[%s@%s %s]* ",username,hostname,cwdname);
  printf("%s",line);
  fflush(stdout);
  sleep(5);
}

int GetUserCommand(char line[], int n)
{
  char* s = fgets(line,n,stdin);
  if(s == NULL)return -1;
  line[strlen(line)-1] = ZERO;
  return strlen(line);
}
char* gArav[NUM];
void SplitCommand(char command[], size_t n)
{
  gArav[0] = strtok(command,SEP);
  int index = 1;
  while((gArav[index++] = strtok(NULL,SEP)));
}


void Die()
{
  exit(errno);
}

const char* Home()
{
  const char* home = getenv("HOME");
  return home;
}
void ExcuteCommand()
{
  pid_t id = fork();
  if(id < 0 ) Die();
  else if(id == 0)
  {
    execvp(gArav[0],gArav);
    exit(errno);
  }else{
    int status = 0;
    pid_t rid = waitpid(id,&status,0);
    if(rid > 0)
    {
      //do other
    }
}
}

void Cd()
{
  const char* path = gArav[1];
  if(path == NULL){
    path = Home();
  }
  chdir(path);

  //刷新环境变量
  char temp[SIZE*2];
  getcwd(temp,sizeof(temp));
  snprintf(cwd,sizeof(cwd),"PWD=%s",temp);
  putenv(cwd);

}



int CheckBUildin()
{
  int yes = 0;
  const char* enter = gArav[0];
  if(strcmp("cd", enter) == 0){
    yes = 1;
    Cd();
  }

}

int main()
{
    while (1) {
        char commandline[SIZE];
        makecommandlineandPrint(commandline, sizeof(commandline));

        char usercommand[SIZE];
        int n = GetUserCommand(usercommand, sizeof(usercommand));
        if (n <= 0) continue;

        // 检查是否输入了退出命令
        if (strcmp(usercommand, "exit") == 0) {
            break;
        }

        SplitCommand(usercommand, sizeof(usercommand));
        ExcuteCommand();
    }

    return 0;
}

