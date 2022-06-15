#pragma once

#if defined(_MSC_VER)
#include "Base/platform/platform_windows.h"
#else
#include "Base/platform/platform_linux.h"
#endif

#if defined(PLATFORM_SDL)
#include <SDL2/SDL_timer.h>
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


inline uint64
Platform_GetPerformanceCounter()
{
#if defined(PLATFORM_SDL)
    return SDL_GetPerformanceCounter();
#else
    return 0;
#endif
}


inline uint64
Platform_GetPerformanceFrequency()
{
#if defined(PLATFORM_SDL)
    return SDL_GetPerformanceFrequency();
#else
    return 0;
#endif
}

// inline Long
// Platform_ToMilliSeconds(ULong counter)
// {
// #if defined(PLATFORM_SDL)
//     return (counter * 1000) / SDL_GetPerformanceFrequency();
// #else
//     return 0;
// #endif
// }


// inline Long
// Platform_ToMicroSeconds(ULong counter)
// {
// #if defined(PLATFORM_SDL)
//     return (counter * 1000000) / SDL_GetPerformanceFrequency();
// #else
//     return 0;
// #endif
// }


inline float
Platform_ToSeconds(ULong counter)
{
    float t = counter;
#if defined(PLATFORM_SDL)
    return t / SDL_GetPerformanceFrequency();
#else
    return 0;
#endif
}