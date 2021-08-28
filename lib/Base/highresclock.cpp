#include "highresclock.h"
#include "typedefs.h"
#if defined(_MSC_VER)
#include <SDL_timer.h>
#else
#include <SDL2/SDL_timer.h>
#endif


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


CounterTimer
CounterTimer_Make(uint64 event_count)
{
    CounterTimer timer{ .event_count   = event_count,
                        .current_count = 0 };
    ElapsedCount(timer.counter);
    return timer;
}


bool
Timer_Poll(CounterTimer& timer)
{
    timer.current_count += ElapsedCount(timer.counter);

    if (timer.current_count > timer.event_count)
    {
        timer.current_count -= timer.event_count;
        return true;
    }
    else
    {
        return false;
    }
}
