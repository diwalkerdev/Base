#include "smallmath.h"

int ftoi(float x)
{
    // Remember just adding +0.5 and then casting doesn't work for negative numbers.
    return static_cast<int>((x + 32768.5) - 32768);
}
