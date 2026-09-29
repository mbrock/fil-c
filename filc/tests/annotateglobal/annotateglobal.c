/* __attribute__((annotate)) on functions and globals (llvm.global.annotations), locals
   (llvm.var.annotation) and fields (llvm.ptr.annotation), and __builtin_annotation
   (llvm.annotation) compile, and the annotated things behave as usual. */
#include <stdfil.h>
#include <stdio.h>
#include <stdlib.h>

struct box {
    int* __attribute__((annotate("field"))) ptr;
};

__attribute__((annotate("realtime"))) int twice(int x)
{
    return 2 * x;
}

__attribute__((annotate("counter"))) int counter;
__attribute__((annotate("table"), annotate("second"))) int* table[4];

int main()
{
    int local __attribute__((annotate("local"))) = twice(21);
    for (int i = 0; i < 4; i++) {
        table[i] = malloc(100 * sizeof(int));
        table[i][99] = i;
    }
    struct box* b = malloc(sizeof(struct box));
    b->ptr = malloc(100 * sizeof(int));
    b->ptr[99] = 7;
    zgc_request_and_wait();
    for (int i = 0; i < 4; i++)
        ZASSERT(table[i][99] == i);
    ZASSERT(b->ptr[99] == 7);
    counter = __builtin_annotation(local, "builtin");
    printf("annotate ok %d\n", counter);
    return 0;
}
