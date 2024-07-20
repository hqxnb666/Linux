#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>

prin(int n)

{

    if (n == 0)

        return;

    int bbb = n % 10;

    prin(n / 10);

    printf("%d", bbb);

}

int main()

{

    int num = 12345;



    prin(num);

    printf("\n");

    return 0;
}