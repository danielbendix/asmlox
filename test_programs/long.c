#include <time.h>
#include <stdio.h>

int main()
{
    clock_t start = clock();
    double n = 0;
    for (double i = 0; i < 100000; i = i + 1) {
        for (double j = 0; j < i; j = j + 1) {
            n = n + j;
        }
    }

    printf("%f\n", ((double)clock() - start) / CLOCKS_PER_SEC);
    printf("%f\n", n);
}
