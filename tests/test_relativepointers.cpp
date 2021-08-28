#include "Base/platform/platform.h"
#include "Base/typedefs.h"
#include <array>
#include <cassert>
#include <concepts>
#include <cstdio>
#include <limits>

//////////////////////////////////////////////////////////////////////////////

template <std::signed_integral Int>
Int
CalcOffset(uint64 a, uint64 b)
{
    // Addresses must be less than 0x7fffffffffffffff.
    // This ensures that the pointers can be safely cast to a signed integer and subtracted.
    // The worst case is 0 - 0x7fffffffffffffff.
    constexpr auto const MAX_ADDR = std::numeric_limits<int64>::max();
    bool                 in_range = (a <= MAX_ADDR) && (b <= MAX_ADDR);

    if (!in_range)
    {
        return 0;
    }
    else
    {
        int64 offset = Cast(int64, b) - Cast(int64, a);

        // Note(DW): Remember you can have a slightly lower value than the max.
        // For a int8, lowest = -0x80 and highest = 0x7F.
        // But for consistency we don't max use of the full lowest range, so our range is from -max to +max
        // and not -max-1 to +max.
        in_range = (offset > std::numeric_limits<Int>::lowest()) && (offset <= std::numeric_limits<Int>::max());

        return in_range ? offset : 0;
    }
}

// Max relative address range is +/-85899345910 Gbs for a rptr<Tp, int64>.
template <typename Tp, std::signed_integral Int>
struct rptr
{
    Int offset;

    rptr()
        : offset(0)
    {
    }

    rptr(Tp* other)
    {
        Set(other);
    }

    void
    Set(Tp* other)
    {
        offset = CalcOffset<Int>((uint64)this, (uint64)other);
    }

    Tp*
    Get()
    {
        return (Tp*)(((int8*)this) + offset);
    }

    Tp&
    operator*()
    {
        return *(((int8*)this) + offset);
    }

    Tp*
    operator->()
    {
        return (Tp*)(((int8*)this) + offset);
    }

    rptr<Tp, Int>
    operator=(Tp* other)
    {
        Set(other);
        return *this;
    }

    operator bool() const
    {
        return offset != 0;
    }

    bool
    IsNull() const
    {
        return offset == 0;
    }

    void
    Reset()
    {
        offset = 0;
    }
};

template <typename Tp>
using rptr64 = rptr<Tp, int64>;

template <typename Tp>
using rptr32 = rptr<Tp, int32>;

template <typename Tp>
using rptr16 = rptr<Tp, int16>;

template <typename Tp>
using rptr8 = rptr<Tp, int8>;

//////////////////////////////////////////////////////////////////////////////

struct Foo
{
    int x;
};

struct DummyData
{
    Foo                 foo;
    rptr8<Foo>          foo_ptr;
    std::array<int8, 4> block;
    rptr32<int8>        ptr;
};

void
Test_LifeCycle()
{
    rptr32<DummyData> ptr;
    assert(ptr == false);
    assert(ptr.IsNull());

    DummyData data;
    ptr.Set(&data);

    assert(ptr == true);
    assert(ptr);

    ptr.Reset();
    assert(ptr == false);
}

void
Test_CanDeReferenceDataInBlock()
{
    auto  OneKBytes = Kilobytes(1);
    void* memory;
    {
        memory = Platform_AllocateVirtualMemory(OneKBytes);
        assert(memory != 0);
    }

    DummyData* data  = Cast(DummyData*, memory);
    auto&      ptr   = data->ptr;
    auto&      block = data->block;
    {
        block[0] = 0;
        block[1] = 1;
        block[2] = 2;
        block[3] = 3;
    }

    ptr.Set(&block[0]);
    assert(*ptr == 0);
    // Note(DW): It possibly doesn't make much sence to check the actual offset as we
    // don't know how how the compiler will layout our data structs.
    // assert(ptr.offset == -8);

    ptr.Set(&block[1]);
    assert(*ptr == 1);

    ptr.Set(&block[2]);
    assert(*ptr == 2);

    ptr.Set(&block[3]);
    assert(*ptr == 3);

    Platform_FreeVirtualMemory(memory, OneKBytes);
}

void
Test_CheckPtrIsNullIfRangeExceedsContainerSize()
{
    printf("Test_CheckPtrIsNullIfRangeExceedsContainerSize\n");
    {
        int8  offset;
        int64 max = std::numeric_limits<int8>::max();

        offset = CalcOffset<int8>(0, max);
        assert(offset == max);

        offset = CalcOffset<int8>(0, max + 1);
        assert(offset == 0);

        offset = CalcOffset<int8>(max, 0);
        assert(offset == -max);

        offset = CalcOffset<int8>(max + 1, 0);
        assert(offset == 0);
    }

    {
        int16 offset;
        int64 max = std::numeric_limits<int16>::max();

        offset = CalcOffset<int16>(0, max);
        assert(offset == max);

        offset = CalcOffset<int16>(0, max + 1);
        assert(offset == 0);

        offset = CalcOffset<int16>(max, 0);
        assert(offset == -max);

        offset = CalcOffset<int16>(max + 1, 0);
        assert(offset == 0);
    }

    {
        int32 offset;
        int64 max = std::numeric_limits<int32>::max();

        offset = CalcOffset<int32>(0, max);
        assert(offset == max);

        offset = CalcOffset<int32>(0, max + 1);
        assert(offset == 0);

        offset = CalcOffset<int32>(max, 0);
        assert(offset == -max);

        offset = CalcOffset<int32>(max + 1, 0);
        assert(offset == 0);
    }

    {
        int64  offset;
        int64  max  = std::numeric_limits<int64>::max();
        uint64 umax = 0x8000000000000000;

        offset = CalcOffset<int64>(0, max);
        assert(offset == max);

        offset = CalcOffset<int64>(0, umax);
        assert(offset == 0);

        offset = CalcOffset<int64>(max, 0);
        assert(offset == -max);

        offset = CalcOffset<int64>(umax, 0);
        assert(offset == 0);
    }
}

void
Test_AssignmentAndDeferencingOperators()
{
    auto  OneKBytes = Kilobytes(1);
    void* memory;
    {
        memory = Platform_AllocateVirtualMemory(OneKBytes);
        assert(memory != 0);
    }

    DummyData* data = Cast(DummyData*, memory);
    // Check assignment.
    data->foo_ptr = &data->foo;

    // Check dereferencing.
    data->foo_ptr->x = 42;
    assert(data->foo.x == 42);

    // Check Get accessor.
    auto* foo = data->foo_ptr.Get();
    foo->x    = 0;
    assert(data->foo.x == 0);
}

void
Test_RelativePointers()
{
    Test_LifeCycle();
    Test_CanDeReferenceDataInBlock();
    Test_CheckPtrIsNullIfRangeExceedsContainerSize();
    Test_AssignmentAndDeferencingOperators();
    printf("TEST RELATIVE POINTERS\n");
}
