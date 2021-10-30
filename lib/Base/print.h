#pragma once
#include <iostream>

template <typename... Args>
void
Print(char const* fmt, Args... args)
{
    std::cout << std::format(fmt, args...);
}

template <typename... Args>
void
PrintLn(char const* fmt, Args... args)
{
    std::cout << std::format(fmt, args...) << "\n";
}

void
PrintLn()
{
    std::cout << "\n";
}
