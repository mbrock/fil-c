#include <stdio.h>

extern long semi_sum(long *p, long n);

int main(void)
{
    long buf[4] = { 1, 2, 3, 4 };
    /* The load is the first statement of a `;`-separated line. */
    printf("%ld\n", semi_sum(buf, 5));
    printf("SHOULD NOT GET HERE\n");
    return 0;
}
