#pragma once

#include "dllexports.h"
#include "typedefs.h"


public_func void*
Windows_AllocateVirtualMemory(uint64 size, uint64 start_addr = 0);


public_func void
Windows_FreeVirtualMemory(void* addr, uint64 size);