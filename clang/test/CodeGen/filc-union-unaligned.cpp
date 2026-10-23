// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -emit-llvm -DCASE=1 -verify=weak -o /dev/null %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -emit-llvm -DCASE=2 -verify=odd -o /dev/null %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -emit-llvm -DCASE=3 -verify=one -o /dev/null %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -emit-llvm -DCASE=4 -verify=seven -o /dev/null %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -emit-llvm -DCASE=5 -verify=internal -o /dev/null %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -emit-llvm -DCASE=6 -verify=mixed -o /dev/null %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -emit-llvm -DCASE=0 -verify=allowed -o /dev/null %s

union Natural { void *pointer; unsigned long bits; };

// We deliberately do not support synthetic pointer words at offsets that are
// misaligned relative to a word-aligned start. Reject with a diagnostic, rather
// than inventing a byte-only byval descriptor. An underaligned union whose
// pointers are aligned relative to its start is allowed (see below); placing it
// at a misaligned offset in a record is not.
#if CASE == 1
union __attribute__((packed)) Weak { void *pointer; unsigned long bits; };
struct Holder { // weak-error {{unaligned pointer-bearing union storage}}
  char prefix;
  Weak weak;
};
Holder holder;
#elif CASE == 2
union __attribute__((packed)) Odd { void *pointer; char bytes[9]; };
struct Pair { // odd-error {{unaligned pointer-bearing union storage}}
  Odd items[2];
};
Pair pair;
#elif CASE == 3
struct __attribute__((packed, aligned(8))) OffsetOne { // one-error {{unaligned pointer-bearing union storage}}
  char prefix;
  Natural choice;
};
OffsetOne atOne;
#elif CASE == 4
struct __attribute__((packed, aligned(8))) OffsetSeven { // seven-error {{unaligned pointer-bearing union storage}}
  char prefix[7];
  Natural choice;
};
OffsetSeven atSeven;
#elif CASE == 5
// Outer alignment does not repair a pointer at byte 1 in an alternative.
struct __attribute__((packed)) Payload { char prefix; void *pointer; };
union __attribute__((aligned(8))) Internal {
  Payload payload; // internal-error {{unaligned pointer-bearing union}}
  void *pointer;
};
Internal internal;
#elif CASE == 6
// Our deliberately conservative boundary covers every pointer in a record
// containing a union, even if that particular union is pointer-free.
union Numeric { unsigned long bits; double number; };
struct __attribute__((packed)) Mixed { // mixed-error {{unaligned pointer-bearing union storage}}
  char prefix;
  void *pointer;
  Numeric choice;
};
Mixed mixed;
#else
// allowed-no-diagnostics
// Packed does not itself mean unsafe: these offsets and strides are aligned.
struct __attribute__((packed, aligned(8))) Allowed {
  Natural choice;
  unsigned long sentinel;
};
Allowed allowed[2];

// Underaligned unions with pointers at word offsets: the types and their
// aligned placements compile, and each pointer-word access checks the actual
// instance alignment at run time. This is systemd's HashmapBase layout.
union __attribute__((packed)) Weak { void *pointer; unsigned long bits; };
Weak weak;
union __attribute__((packed)) Odd { void *pointer; char bytes[9]; };
Odd odd[2];
struct __attribute__((packed)) Indirect {
  void *storage;
  unsigned char key[16];
  unsigned entries, buckets, lowest;
  unsigned char pad[3];
};
struct Direct { unsigned char storage[sizeof(Indirect)]; };
struct HashmapBase {
  const void *ops;
  union __attribute__((packed)) {
    Indirect indirect;
    Direct direct;
  };
  unsigned type : 2;
  bool hasIndirect : 1;
};
HashmapBase hashmaps[2];

// Pointer-free packed unions retain their ordinary layout and remain allowed.
union __attribute__((packed)) Numeric { long bits; double number; char bytes[9]; };
Numeric numeric;
#endif
