#pragma once

#include "dllexports.h"
#include "typedefs.h"
#if defined(_MSC_VER)
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

struct HighResClock
{
    uint64 current_time;
    uint64 new_time;
};

struct HighResCounter
{
    uint64 current_time;
    uint64 new_time;
};

struct CounterTimer
{
    uint64 const   event_count;
    uint64         current_count;
    HighResCounter counter;
};

public_func double
ElapsedTime(HighResClock& clk);

public_func uint64
ElapsedCount(HighResCounter& clk);

public_func CounterTimer
CounterTimer_Make(uint64 event_count);

public_func bool
Timer_Poll(CounterTimer& timer);
