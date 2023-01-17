// https:// atcoder.jp/contests/abc122/tasks/abc122_b
#include <stdio.h>
int main()
{
    char a[10];
    int countlocal = 0;
    int count = 0;
    char b[4] = {'A', 'T', 'G', 'C'};

    /*for(int i=0; i<10; i++){
    a[i]=NULL;
    }*/

    scanf("%s", a);

    for (int i = 0; i < 10; i++)
    {
        countlocal = 0;
        for (int j = i; j < 10; j++)
        {
            if (a[j] == b[0] | a[j] == b[1] | a[j] == b[2] | a[j] == b[3])
            {
                countlocal++;
            }
            else
            {
                break;
            }
        }
        if (countlocal > count)
        {
            count = countlocal;
        }
    }

    // 出力
    // printf("%s", a);
    printf("%d", count);
    return 0;
}