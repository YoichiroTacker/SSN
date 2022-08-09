// https : // judge.u-aizu.ac.jp/onlinejudge/description.jsp?id=ITP1_7_B&lang=ja
#include <stdio.h>
int main()
{
    int a, b;
    int count = 0;
    // スペース区切りの整数の入力
    scanf("%d %d", &a, &b);

    for (int i = 1; i < a + 1; i++)
    {
        for (int j = i + 1; j < a + 1; j++)
        {
            for (int k = j + 1; k < a + 1; k++)
            {
                if (i + j + k == b)
                {
                    count++;
                }
            }
        }
    }

    // 出力
    printf("%d", count);
    return 0;
}