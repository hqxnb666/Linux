#include <stdio.h>
#include <unistd.h>
int main()
{
int cnt = 10;
while(cnt >= 0)
{
  printf("倒计时：%2d\r",cnt );
  fflush(stdout);
  cnt--;
  sleep(1);
}
printf("\n");
  return 0;
}
