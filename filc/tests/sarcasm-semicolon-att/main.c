#include <stdio.h>
#include <string.h>

extern long semi_sum(long *p, long n);
extern long semi_inc(long *p);
extern long *semi_load_ptr(long **slot);
extern const char semi_str[];

int main(void)
{
    long buf[4] = { 1, 2, 3, 4 };
    long *slot = &buf[2];
    if (semi_sum(buf, 4) != 10 || semi_sum(buf, 0) != 0) {
        printf("FAIL: sum\n");
        return 1;
    }
    if (semi_inc(&buf[0]) != 2 || buf[0] != 2) {
        printf("FAIL: inc\n");
        return 1;
    }
    if (*semi_load_ptr(&slot) != 3) {
        printf("FAIL: load ptr\n");
        return 1;
    }
    if (strcmp(semi_str, "a;b#c\";d")) {
        printf("FAIL: string\n");
        return 1;
    }
    printf("semicolon att ok\n");
    return 0;
}
