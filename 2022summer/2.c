// https://atcoder.jp/contests/abc106/tasks/abc106_b
#include <stdio.h>
int main()
{
    int a;
    int countlocal = 0;
    int count = 0;

    // スペース区切りの整数の入力
    scanf("%d", &a);

    for (int i = 1; i < a + 1; i++)
    {
        countlocal = 0;
        for (int j = 1; j < a + 1; j++)
        {
            if (i % j == 0)
            {
                countlocal++;
            }
        }
        if (countlocal == 8 && i % 2 == 1)
        {
            count++;
        }
    }

    // 出力
    printf("%d", count);
    return 0;
}