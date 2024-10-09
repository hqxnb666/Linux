#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
const char *filename = "log.txt";

int main()
{
  struct stat st;
  int n   = stat(filename,&st);
  if(n<0) return 1;
  printf("file size: %lu\n",st.st_size);
  int fd = open(filename,O_RDONLY);
  if(fd <0)
  {
    perror("open");
    return 2;
  }
  printf("fd: %d\n",fd);
  char* file_buffer = (char*)malloc(st.st_size+1);
  free(file_buffer);
  n = read(fd,file_buffer,st.st_size);
  //  n大于0就是read读到了多少个字节就返回多少
  //== 0 就是读到了结尾
  if(n>0)
  {
    file_buffer[n] = '\0';
    printf("%s\n",file_buffer);
  }
  free(file_buffer);
  close(fd);
  return 0;
}
