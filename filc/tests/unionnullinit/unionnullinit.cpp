// Regression coverage for initialization of pointer-bearing union storage.
// In particular, a null data-member pointer is not necessarily represented by
// all-zero bytes (on this ABI it is all ones); bytewise clearing or repacking
// the wrong range can therefore turn a valid null into a non-null value.

struct MemberOwner {
    int value;
};

using DataMemberPointer = int MemberOwner::*;

union MemberPointerChoice {
    DataMemberPointer member;
    void* pointer;
    unsigned char bytes[sizeof(DataMemberPointer)];
};

// Static zero-initialization activates the first union member. Do not read the
// alternate byte-array member: it is inactive, and its representation is not
// the subject of this test.
MemberPointerChoice staticChoice;
thread_local MemberPointerChoice threadChoice;

#pragma pack(push, 1)
struct PackedOffsetOne {
    unsigned char lead;
    MemberPointerChoice choice;
    unsigned char sentinel;
};

struct PackedOffsetSeven {
    unsigned char lead[7];
    MemberPointerChoice choice;
    unsigned char sentinel;
};

// The active member's null data-member pointer crosses the normalized word
// boundary at byte 8. Its last byte lies in the union's partial-word tail.
struct PackedTailPayload {
    unsigned char lead;
    DataMemberPointer member;
    unsigned char tail[2];
};

union PackedTailChoice {
    PackedTailPayload payload;
    void* pointer;
};
#pragma pack(pop)

static_assert(__builtin_offsetof(PackedOffsetOne, choice) == 1);
static_assert(__builtin_offsetof(PackedOffsetSeven, choice) == 7);
static_assert(__builtin_offsetof(PackedTailPayload, member) == 1);
static_assert(sizeof(PackedTailPayload) == 11);
static_assert(sizeof(PackedTailChoice) == 11);

struct NestedArrayAndBase : MemberOwner {
    unsigned char lead;
    MemberPointerChoice choices[2];
    unsigned char sentinel;
};

struct AnonymousHolder {
    unsigned char lead;
    union {
        DataMemberPointer member;
        void* pointer;
        unsigned char bytes[sizeof(DataMemberPointer)];
    };
    unsigned char sentinel;
};

// An unnamed aggregate is still the first member to initialize. The member
// at byte 8 also catches confusing a field's bit offset with a byte offset.
union AnonymousRecordChoice {
    struct {
        unsigned long lead;
        DataMemberPointer member;
    };
    void* pointer;
};

struct MemberPointerBase { DataMemberPointer inherited; };
struct MembersWithBase : MemberPointerBase { DataMemberPointer own; };
union RecordChoice { MembersWithBase members; void* pointer; };

// Static aggregate zero-initialization also initializes the first member of
// each nested union, including unions in arrays, a base-containing object, and
// an anonymous union.
PackedOffsetOne staticAtOne;
PackedOffsetSeven staticAtSeven;
PackedTailChoice staticTail;
NestedArrayAndBase staticNested;
AnonymousHolder staticAnonymous;
AnonymousRecordChoice staticAnonymousRecord;
RecordChoice staticRecord;
thread_local PackedOffsetOne threadAtOne;
thread_local PackedOffsetSeven threadAtSeven;
thread_local PackedTailChoice threadTail;
thread_local NestedArrayAndBase threadNested;

// Dynamically initialized, lifetime-extended temporaries are allocated before
// their null initializer is emitted. The initializer may have a different LLVM
// storage type, but must not change the global's size or its C++ access type.
struct DynamicRecord {
    DataMemberPointer member;
    void* pointer;
    explicit DynamicRecord(int) : member(nullptr), pointer(nullptr) {}
};
union DynamicChoice {
    DataMemberPointer member;
    void* pointer;
    explicit DynamicChoice(int) : member(nullptr) {}
};
int runtimeValue = 17;
const DynamicRecord& recordTemporary = DynamicRecord(runtimeValue);
const DynamicChoice& unionTemporary = DynamicChoice(runtimeValue);

int copyCount;
struct __attribute__((packed)) CountCopies {
    CountCopies() = default;
    CountCopies(const CountCopies&) { ++copyCount; }
    CountCopies& operator=(const CountCopies&) { ++copyCount; return *this; }
};
struct __attribute__((packed, aligned(16))) GroupedCopy {
    unsigned char lead;
    MemberPointerChoice choice;
    unsigned char sentinel;
    CountCopies counter;
    GroupedCopy() : lead(0x39), choice{nullptr}, sentinel(0xe7) {}
    GroupedCopy(const GroupedCopy&) = default;
    GroupedCopy& operator=(const GroupedCopy&) = default;
};

static bool isNull(DataMemberPointer pointer)
{
    return pointer == nullptr;
}

static bool checkStaticObjects()
{
    return isNull(staticChoice.member) &&
           isNull(staticAtOne.choice.member) &&
           staticAtOne.lead == 0 && staticAtOne.sentinel == 0 &&
           isNull(staticAtSeven.choice.member) &&
           staticAtSeven.lead[0] == 0 && staticAtSeven.lead[6] == 0 &&
           staticAtSeven.sentinel == 0 &&
           isNull(staticTail.payload.member) &&
           staticTail.payload.lead == 0 && staticTail.payload.tail[1] == 0 &&
           isNull(staticNested.choices[0].member) &&
           isNull(staticNested.choices[1].member) &&
           staticNested.lead == 0 && staticNested.sentinel == 0 &&
           isNull(staticAnonymous.member) &&
           staticAnonymous.lead == 0 && staticAnonymous.sentinel == 0 &&
           isNull(staticAnonymousRecord.member) &&
           staticAnonymousRecord.lead == 0 &&
           isNull(staticRecord.members.inherited) &&
           isNull(staticRecord.members.own);
}

static bool checkThreadLocalObjects()
{
    return isNull(threadChoice.member) &&
           isNull(threadAtOne.choice.member) &&
           threadAtOne.lead == 0 && threadAtOne.sentinel == 0 &&
           isNull(threadAtSeven.choice.member) &&
           threadAtSeven.lead[0] == 0 && threadAtSeven.lead[6] == 0 &&
           threadAtSeven.sentinel == 0 &&
           isNull(threadTail.payload.member) &&
           threadTail.payload.tail[1] == 0 &&
           isNull(threadNested.choices[0].member) &&
           isNull(threadNested.choices[1].member) &&
           threadNested.lead == 0 && threadNested.sentinel == 0;
}

int main()
{
    if (!checkStaticObjects() || !checkThreadLocalObjects())
        return 1;
    if (!isNull(recordTemporary.member) || recordTemporary.pointer != nullptr ||
        !isNull(unionTemporary.member))
        return 3;

    // Explicit first-member initialization exercises the ordinary aggregate
    // initialization path too, without ever reading an inactive union member.
    MemberPointerChoice initialized { nullptr };
    PackedOffsetOne packedInitialized { 0, { nullptr }, 0 };
    if (!isNull(initialized.member) ||
        !isNull(packedInitialized.choice.member) ||
        packedInitialized.lead != 0 || packedInitialized.sentinel != 0)
        return 2;

    // Defaulted C++ copies group trivial fields before invoking the nontrivial
    // member's copy operation. The first field is a byte, not the union.
    GroupedCopy source;
    GroupedCopy constructed(source);
    GroupedCopy assigned;
    assigned = source;
    if (copyCount != 2 || !isNull(constructed.choice.member) ||
        !isNull(assigned.choice.member) || constructed.lead != 0x39 ||
        assigned.sentinel != 0xe7)
        return 4;

    // Bitcasts must retain the packed source/destination storage types until
    // the copy decision. All fields here have initialized representations.
    struct Bytes { unsigned char data[sizeof(PackedOffsetOne)]; };
    Bytes bytes = __builtin_bit_cast(Bytes, packedInitialized);
    PackedOffsetOne bitcastCopy = __builtin_bit_cast(PackedOffsetOne, bytes);
    if (!isNull(bitcastCopy.choice.member) || bitcastCopy.lead != 0 ||
        bitcastCopy.sentinel != 0)
        return 5;

    return 0;
}
