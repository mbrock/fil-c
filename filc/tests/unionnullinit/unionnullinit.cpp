// The first member's null representation must survive pointer-word storage.
// In the Itanium ABI a null data-member pointer is -1, not all-zero bytes.
#include <cassert>

struct Owner { int value; };
using Member = int Owner::*;
union Choice { Member member; void* pointer; };

Choice staticChoice;
thread_local Choice threadChoice;
struct Nested : Owner { char lead; Choice choices[2]; char sentinel; };
Nested staticNested;
thread_local Nested threadNested;

struct AnonymousHolder {
    char lead;
    union { Member member; void* pointer; };
    char sentinel;
};
AnonymousHolder staticAnonymous;

// The anonymous aggregate is still the first member. Its data-member pointer
// at byte 8 catches confusing AST field bit offsets with byte offsets.
union AnonymousChoice {
    struct { unsigned long lead; Member member; };
    void* pointer;
};
AnonymousChoice staticAnonymousChoice;
struct Base { Member inherited; };
struct WithBase : Base { Member own; };
union RecordChoice { WithBase members; void* pointer; };
RecordChoice staticRecord;

// A lifetime-extended temporary is allocated before its null initializer is
// emitted. Differently typed initializers must keep its size and access type.
union Dynamic {
    Member member;
    void* pointer;
    explicit Dynamic(int) : member(nullptr) {}
};
int runtimeValue = 17;
const Dynamic& temporary = Dynamic(runtimeValue);

int copyCount;
struct Counter {
    Counter() = default;
    Counter(const Counter&) { ++copyCount; }
    Counter& operator=(const Counter&) { ++copyCount; return *this; }
};
struct Grouped {
    char lead;
    Choice choice;
    char sentinel;
    Counter counter;
    Grouped() : lead(0x39), choice{nullptr}, sentinel(0x67) {}
    Grouped(const Grouped&) = default;
    Grouped& operator=(const Grouped&) = default;
};

int main()
{
    assert(staticChoice.member == nullptr && threadChoice.member == nullptr);
    assert(staticNested.choices[0].member == nullptr &&
           staticNested.choices[1].member == nullptr);
    assert(threadNested.choices[0].member == nullptr &&
           threadNested.choices[1].member == nullptr);
    assert(staticNested.lead == 0 && staticNested.sentinel == 0);
    assert(staticAnonymous.member == nullptr && staticAnonymous.sentinel == 0);
    assert(staticAnonymousChoice.lead == 0 &&
           staticAnonymousChoice.member == nullptr);
    assert(staticRecord.members.inherited == nullptr &&
           staticRecord.members.own == nullptr);
    assert(temporary.member == nullptr);

    Choice initialized{nullptr};
    assert(initialized.member == nullptr);
    Grouped source, assigned;
    Grouped constructed(source);
    assigned = source;
    assert(copyCount == 2);
    assert(constructed.choice.member == nullptr && assigned.choice.member == nullptr);
    assert(constructed.lead == 0x39 && assigned.sentinel == 0x67);

    // Bitwise representation changes must retain a nonzero member-pointer null.
    struct Bytes { unsigned char data[sizeof(Choice)]; };
    Bytes bytes = __builtin_bit_cast(Bytes, initialized);
    Choice recovered = __builtin_bit_cast(Choice, bytes);
    assert(recovered.member == nullptr);
}
