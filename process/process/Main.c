#include <stdio.h>
#include <unistd.h>
#include "Processbar.h"
    double bandwidth = 1024*1024*1.0;
  void download(double filesize,callback_t cb)
  {
    double current  = 0.0;

    printf("download begin, filesize: %lf\n",current);
    while(current <= filesize)
    {
        cb(filesize,current);
        current += bandwidth;
        usleep(10000);
    }
    printf("\ndownload done, filesize: %lf\n",filesize);
  }
int main()
{
  //ForTest();
  download(100*1024*1024,ProcBar);
  
  //ProcBar(100.0,56.9);
  //ProcBar(100.0,2.9);
  
  return 0;
}
