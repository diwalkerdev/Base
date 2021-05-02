#pragma once

#if defined(_MSC_VER)
#include "linux/platform_windows.h"
#else
#include "linux/platform_linux.h"
#endif


inline void*
Platform_AllocateVirtualMemory(uint64 size, uint64 start_addr = 0)
{
#if defined(_MSC_VER)
    return Windows_AllocateVirtualMemory(size, start_addr);
#else
    return Linux_AllocateVirtualMemory(size, start_addr);
#endif
}


inline void
Platform_FreeVirtualMemory(void* addr, uint64 size)
{
#if defined(_MSC_VER)
    Windows_FreeVirtualMemory(addr, size);
#else
    Linux_FreeVirtualMemory(addr, size);
#endif
}