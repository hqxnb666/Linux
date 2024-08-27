#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#define SIZE 512


  const char* getusername()
  {
    const char* name = getenv("USER");
    if(name == NULL) return "NONE";
    return name;
  }
const char* gethostname()
{
  const char* hostname = getenv("HOSTNAME");
  if(name == NULL) return "NONE";
  return hostname;
}
int main()
 {

  
  char output[SIZE];
  printf("name : %s,hostname : %s\n",getusername());
 return 0;
 }
