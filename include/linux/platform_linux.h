#pragma once
#include "typedefs.h"
#include <cassert>
#include <sys/mman.h>

inline void*
Linux_AllocateVirtualMemory(uint64 size, uint64 start_addr = 0)
{
    auto* region = mmap((void*)start_addr,
                        size,
                        PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANON,
                        -1,
                        0);

    assert(region);
    return region;
}

inline void
Linux_FreeVirtualMemory(void* addr, uint64 size)
{
    munmap(addr, size);
}