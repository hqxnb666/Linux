#include "Processbar.h"
#include <string.h>
#include <unistd.h>
#define Length 101
#define Style '='
const char *label = "|/-\\";
// I printf("[%-100s][%.1lf%%][%c]\r",bar,rate,label[cnt%len]);void  ProcBar()
// {
//  char bar[Length];
//  int len = strlen(label);
//  memset(bar, '\0', sizeof(bar));
//  int cnt = 0;
//  while(cnt <= 100)
//  a
//    printf("[%-100s][%3d%%][%c]\r",bar,cnt,label[cnt%len]);
//    fflush(stdout);
//    bar[cnt++] = Style;
//    usleep(10000);
//
//  }
}a


 void  ProcBar(double total, double current)
{
  char bar[Length];
  int len = strlen(label);
   memset(bar, '\0', sizeof(bar));
  int cnt = 0;
  double rate = (current * 100.0)/total;
  int loop_count = (int)rate;
  while(cnt <= loop_count)
  {
    printf("[%-100s][%.1lf%%][%c]\r",bar,rate,label[cnt%len]);
    fflush(stdout);
    bar[cnt++] = Style;
  } 
}
