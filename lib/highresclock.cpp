#include "highresclock.h"

double elapsed_time(HighResClock& clk)
{
    using duration = std::chrono::duration<double>;
    double time_secs;

    clk.new_time     = high_res_clock::now();
    time_secs        = duration(clk.new_time - clk.current_time).count();
    clk.current_time = clk.new_time;
    return time_secs;
}
