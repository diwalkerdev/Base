#include <cassert>

void test_cast_float_to_int_rounds_to_nearest_integer()
{
    float value = 1.51;
    float half  = 0.5;

    long v = (long)value;
    long h = (long)half;

    long minus_bit = v & 0x80000000;
    h |= minus_bit;

    int result = (int)(value + float(h));

    assert(result == 2);
}