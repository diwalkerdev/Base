#pragma once

#include "dllexports.h"
#include <chrono>

using high_res_clock = std::chrono::high_resolution_clock;

struct HighResClock {
    high_res_clock::time_point current_time;
    high_res_clock::time_point new_time;
};

DLL_PUBLIC
auto elapsed_time(HighResClock& clk) -> double;
