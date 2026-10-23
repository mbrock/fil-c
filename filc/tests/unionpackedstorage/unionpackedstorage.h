#include <stdint.h>

/* systemd's HashmapBase: a packed (alignment 1) pointer-bearing union whose
 * pointers are word-aligned relative to its start, at offset 8 of an aligned
 * record. Direct storage holds pointer-bearing entries in its byte array. */
struct __attribute__((packed)) Indirect {
    void *storage;
    uint8_t key[16];
    unsigned entries, buckets, lowest;
    uint8_t pad[3];
};

struct Direct { uint8_t storage[sizeof(struct Indirect)]; };

struct Base {
    const void *ops;
    union __attribute__((packed)) {
        struct Indirect indirect;
        struct Direct direct;
    };
    unsigned type : 2;
    unsigned has_indirect : 1;
};

union __attribute__((packed)) Odd { int *pointer; char bytes[9]; };

void copy_base(struct Base *out, const struct Base *in);
union Odd round_trip(union Odd value);
int check_varargs(int tag, ...);
