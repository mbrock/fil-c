#ifndef UNION_RECORD_SCALAR_VALUE_H
#define UNION_RECORD_SCALAR_VALUE_H

union Scalar {
    long integer;
    double number;
    void* pointer;
    int (*function)(int);
};

struct First { union Scalar value; long tag; };
struct Last { long tag; union Scalar value; };
struct Array { union Scalar values[1]; long tag; };
struct Hidden { union { long integer; int* pointers[1]; } value; long tag; };
struct Wide { union { void* pointer; int* pointers[2]; } value; };
struct Large { union { void* pointer; __int128 integer; } value; };

union Scalar scalar(union Scalar value);
struct First first(struct First value);
struct Last last(struct Last value);
struct Array array(struct Array value);
struct Hidden hidden(struct Hidden value);
struct Wide wide(struct Wide value);
struct Large large(struct Large value);
void variadic(int* expected, ...);

#endif
