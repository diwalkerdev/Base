#include "containers/array.h"

void
Test_Array()
{
    Array<int, 3> data;

    // You can write to any element at any time.
    data[0] = 0;
    data[1] = 1;
    data[2] = 2;

    assert(data[1] = 1);

    // However, until reserve is called, the size will be 0.
    assert(data.size() == 0);
    assert(data.begin() == data.end());

    // reserve returns a bool to indicate if the reservation was successful or not.
    assert(data.reserve(3));
    assert(data.size() == 3);

    // There are free function equivalents for most member functions.
    assert(Array_Size(data) == 3);

    // Onced reserved you can now iterate the data.
    auto it = data.begin();

    assert(*it++ == 0);
    assert(*it++ == 1);
    assert(*it++ == 2);

    it = data.end() - 1;

    assert(*it == 2);
    assert((data.end() - data.begin()) == 3);

    int test_value = 0;
    for (auto value : data)
    {
        assert(value == test_value);
        test_value += 1;
    }


    data.back() = 3;
    assert(data.back() == 3);

    assert(!data.reserve(1));
}