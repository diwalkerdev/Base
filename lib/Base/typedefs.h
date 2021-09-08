#pragma once

#include <inttypes.h>

using int8    = int8_t;
using int16   = int16_t;
using int32   = int32_t;
using int64   = int64_t;
using uint8   = uint8_t;
using uint16  = uint16_t;
using uint32  = uint32_t;
using uint64  = uint64_t;
using cstring = char const*;


constexpr uint64
Bytes(uint64 value)
{
    return value;
}


constexpr uint64
Kilobytes(uint64 value)
{
    return value * 1024;
}


constexpr uint64
Megabytes(uint64 value)
{
    return value * 1024 * 1024;
}


constexpr uint64
Gigabytes(uint64 value)
{
    return value * 1024 * 1024 * 1024;
}


#define Cast(type, variable) static_cast<type>((variable))
