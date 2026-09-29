/* An annotated global keeps its bounds. */
__attribute__((annotate("table"))) char table[32];
__attribute__((annotate("after"))) char after[32];

int main()
{
    volatile int index = 40;
    table[index] = 1;
    return 0;
}
