#pragma once

#include "dllexports.h"
#include "typedefs.h"
#include <SDL2/SDL.h>

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

public_func double
ElapsedTime(HighResClock& clk);

public_func uint64
ElapsedCount(HighResCounter& clk);