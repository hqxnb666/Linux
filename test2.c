#include <stdio.h>

int Add(int a, int b)
{
  int sum = 0;
  for(int i = a ; i<=b; i++)
  {
    sum += i;
  }
  return sum;
}
int main()
{
printf("run begin...\n");
int result = 0;
result = Add(1,100);
printf("result: %d\n",result);

printf("run end\n");
  return 0;
}
