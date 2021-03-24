#include "highresclock.h"
#include "typedefs.h"
#include <SDL2/SDL_timer.h>

double
ElapsedTime(HighResClock& clk)
{
    clk.new_time = SDL_GetPerformanceCounter();

    auto time_secs = (clk.new_time - clk.current_time) / (double)SDL_GetPerformanceFrequency();

    clk.current_time = SDL_GetPerformanceCounter();

    return time_secs;
}

uint64
ElapsedCount(HighResCounter& clk)
{
    clk.new_time = SDL_GetPerformanceCounter();

    uint64 delta_count = (clk.new_time - clk.current_time);

    clk.current_time = SDL_GetPerformanceCounter();

    return delta_count;
}
