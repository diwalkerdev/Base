#pragma once
#include "dllexports.h"
#include "typedefs.h"

// DLL_PUBLIC int cast_ftoi_2(float x);

// DLL_PUBLIC int cast_ftoi(float x);

inline int cast_ftoi_2(float x)
{
    return static_cast<int>(x < 0 ? x - 0.5 : x + 0.5);
}

#define FLOAT_TO_BITS(v) *(int32*)(&(v))
#define BITS_TO_FLOAT(v) *(float*)(&(v))

static constexpr float Half { 0.5 };

inline int cast_ftoi(float value)
{
    // This only works if float is exactly 32 bits.
    // static_assert(sizeof(float) == sizeof(int32));

    // +0.5 if value is positive.
    // -0.5 if value is negative.
    int32 int_half = (FLOAT_TO_BITS(Half) | (FLOAT_TO_BITS(value) & 0x80000000)); // rhs of | is the minus bit.

    return (int32)(value + BITS_TO_FLOAT(int_half));
}
