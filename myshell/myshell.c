#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SIZE 512
#define ZERO '\0'
#define SEP " "
#define NUM 32
#define SkipPath(p) do{ p += (strlen(p)-1); while(*p != '/') p--; }while(0)
#define SkipSpace(cmd, pos) do{\
    while(1){\
        if(isspace(cmd[pos]))\
            pos++;\
        else break;\
    }\
}while(0)

// "ls -a -l -n > myfile.txt"
#define None_Redir 0
#define In_Redir   1
#define Out_Redir  2
#define App_Redir  3

int redir_type = None_Redir;
char* filename = NULL;

char cwd[SIZE * 2];
char* gArgv[NUM];
int lastcode = 0;

void Die()
{
 exit(-1);   
}

const char* GetHome()
{
   const char* home = getenv("HOME");
   if(home == NULL) return "/";
   return home;
}

const char* GetUserName()
{
   
   const char* username = getenv("USER");
   if(username == NULL) return "/";
   return username;
}
const char* GetHostName()
{
   
   const char* hostname = getenv("HOSTNAME");
   if(hostname == NULL) return "/";
   return hostname;
}
// 临时
const char* GetCwd()
{
   const char* cwd = getenv("PWD");
   if(cwd == NULL) return "/";
   return cwd;
}

// commandline : output
void MakeCommandLineAndPrint()
{
  char line[SIZE];
  const char* username = GetHome();
  const char* hostname= GetHostName();
  const char* cwd = GetCwd();
  SkipPath(cwd);
  snprintf(line,sizeof(line),"[%s@%s %s]>",username,hostname,strlen(cwd) == 1 ? "/" : cwd+1);
  printf("%s",line);
  fflush(stdout);
}

int GetUserCommand(char command[], size_t n)
{
  char *s = fgets(command,n,stdin);
  if(s == NULL) return -1;
  command[strlen(command)-1] = ZERO;
  return strlen(command);
}


void SplitCommand(char command[], size_t n)
{
   (void)n;
   gArgv[0] = strtok(command,SEP);
   int index = 1;
   while((gArgv[index++] = strtok(NULL,SEP)));
  }


void ExecuteCommand()
{
   pid_t id = fork();
	 if(id < 0)
		{
			Die();
		}else if(id == 0)
    {
      if(redir_type == In_Redir)
      {
        int fd = open(filename,O_RDONLY);
        dup2(fd,0);
      }
      else if(redir_type == Out_Redir)
      {
        int fd = open(filename,O_WRONLY | O_CREAT | O_TRUNC,0666);
        dup2(fd,1);
      }
      else if(redir_type == App_Redir)
      {
        int fd = open(filename,O_WRONLY | O_CREAT | O_APPEND);
        dup2(fd,1);
      }
      else{

      }

    execvp(gArgv[0],gArgv);
    exit(errno);
    }
    else 
    {
      int status = 0;
      pid_t rid = waitpid(id,&status,0);
      if (rid > 0)
{
    if (WIFEXITED(status)) {
        lastcode = WEXITSTATUS(status);
        if(lastcode != 0) printf("%s:%s:%d\n", gArgv[0], strerror(lastcode), lastcode);
    } else if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        printf("%s: terminated by signal %d\n", gArgv[0], sig);
    }
}

    }

}   


void Cd()
{
   
}

int CheckBuildin()
{
     
}

void CheckRedir(char cmd[])
{
  int pos = 0;
  int end = strlen(cmd);
  while(pos < end)
  {
    if(cmd[pos] == '>'){
      if(cmd[pos+1] == '>')
      {
        cmd[pos++] = 0;
        pos++;
        redir_type = App_Redir;
        SkipSpace(cmd,pos);
        filename = cmd + pos;
      }
      else{
        cmd[pos++] = 0;
        redir_type = Out_Redir;
        SkipSpace(cmd,pos);
        filename = cmd + pos;
      }
    }
    else if(cmd[pos] == '<'){
        cmd[pos++] = 0;
            redir_type = In_Redir;
            SkipSpace(cmd, pos);
            filename = cmd + pos;

    }
		else
		{
			pos++;
  }
}
}
int main()
{
    int quit = 0;
    while(!quit)
    {
        // 0. 重置
        redir_type = None_Redir;
        filename = NULL;
        // 1. 我们需要自己输出一个命令行
        MakeCommandLineAndPrint();

        // 2. 获取用户命令字符串
        char usercommand[SIZE];
        int n = GetUserCommand(usercommand, sizeof(usercommand));
        if(n <= 0) return 1;

        // 2.1 checkredir
        CheckRedir(usercommand);

        // 2.2 debug
//        printf("cmd: %s\n", usercommand);
//        printf("redir: %d\n", redir_type);
//        printf("filename: %s\n", filename);
//
        // 3. 命令行字符串分割. 
        SplitCommand(usercommand, sizeof(usercommand));

        // 4. 检测命令是否是内建命令
        n = CheckBuildin();
        if(n) continue;
        // 5. 执行命令
        ExecuteCommand();
    }
    return 0;
}

