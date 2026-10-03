/* A pointer-free byval descriptor is still a checked whole-object read. It
 * must not let the caller snapshot 16 bytes from an 8-byte allocation. */
struct __attribute__((packed)) Payload {
    unsigned char prefix[7];
    int *pointer;
};
union __attribute__((packed)) Choice {
    struct Payload payload;
    unsigned char bytes[15];
};
struct __attribute__((aligned(8))) Wrapper {
    unsigned char tag;
    union Choice choice;
};
_Static_assert(sizeof(struct Wrapper) == 16, "transport extent");

__attribute__((noinline)) int consume(struct Wrapper value)
{
    return *value.choice.payload.pointer;
}

static const unsigned long short_object = 0;
int main(void)
{
    return consume(*(const struct Wrapper *)&short_object);
}
