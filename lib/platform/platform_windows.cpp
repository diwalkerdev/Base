#include "linux/platform_windows.h"

#include <windows.h>

#include <memoryapi.h>
#include <cassert>


void*
Windows_AllocateVirtualMemory(uint64 size, uint64 start_addr)
{
    auto* region = VirtualAlloc((void*)start_addr,
                        size,
                        MEM_COMMIT | MEM_RESERVE, // allocation type
                        PAGE_READWRITE); // protect

    assert(region);
    return region;
}


void
Windows_FreeVirtualMemory(void* addr, uint64 size)
{
    auto result = VirtualFree(addr, 
                              size,
                              MEM_RELEASE);
    assert(result == 0);
}