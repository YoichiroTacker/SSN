#include <stdio.h>
#include <stdlib.h>
int main()
{

    int n;
    scanf("%d", &n);
    int s[n];
    for (int i = 0; i < n; i++)
    {
        scanf("%d", &s[i]);
    }
    int q;
    scanf("%d", &q);
    int t[q];
    for (int i = 0; i < q; i++)
    {
        scanf("%d", &t[i]);
    }
    int c;

    // 出力
    for (int i = 0; i < n; i++)
    {
        printf("%d ", s[i]);
    }

    printf("\n%d", n);
    return 0;
}