#include "containers/backfill_vector.hpp"
#include <cassert>


auto make_bfv_12345() -> backfill_vector<int, 5>
{
    backfill_vector<int, 5> bfv;
    bfv.allocate();
    bfv.allocate();
    bfv.allocate();
    bfv.allocate();
    bfv.allocate();
    int index = 1;
    for (int& item : bfv)
    {
        item = index;
        index += 1;
    }
    return bfv;
}

void test_starts_empty()
{
    backfill_vector<int, 5> bfv;
    assert(bfv.size() == 0);
    assert(bfv.capacity() == 5);
}

void test_size_increases_when_items_allocated()
{
    backfill_vector<int, 5> bfv;

    bfv.allocate();
    assert(bfv.size() == 1);

    bfv.allocate();
    bfv.allocate();
    bfv.allocate();

    bfv.allocate();
    assert(bfv.size() == 5);
}

void test_size_decreases_when_items_removed()
{
    auto bfv = make_bfv_12345();

    assert(bfv.size() == 5);
    bfv.remove(4);
    assert(bfv.size() == 4);
    bfv.remove(3);
    assert(bfv.size() == 3);
    bfv.remove(2);
    assert(bfv.size() == 2);
    bfv.remove(1);
    assert(bfv.size() == 1);
    bfv.remove(0);
    assert(bfv.size() == 0);
}

void test_riterators()
{
    auto bfv = make_bfv_12345();
    auto rb  = bfv.rbegin();
    auto re  = bfv.rend();

    assert(*(rb - 0) == 5);
    assert(*(re - 1) == 1);

    bfv.remove(4);
    rb = bfv.rbegin();
    re = bfv.rend();
    assert(*(re - 1) == 1);
    assert(*(rb - 0) == 4);

    bfv.remove(3);
    rb = bfv.rbegin();
    re = bfv.rend();
    assert(*(re - 1) == 1);
    assert(*(rb - 0) == 3);
}


void test_vector_back_fills_when_items_removed()
{
    auto bfv = make_bfv_12345();

    // Perform some basic checks on the iterators.
    assert(bfv.at(4) == 5);
    assert((*(bfv.end() - 1)) == 5);
    assert((*bfv.begin()) == 1);

    // 1234X
    // 1234
    bfv.remove(4);
    assert(bfv.at(3) == 4);

    // 1X3(4)
    // 143
    bfv.remove(1);
    assert(bfv.at(1) == 4);

    // X4(3)
    // 34
    bfv.remove(0);
    assert(bfv.at(0) == 3);

    // X(4)
    // 4
    bfv.remove(0);
    assert(bfv.at(0) == 4);
}


void test_remove_from_list_of_indices()
{
    auto bfv = make_bfv_12345();
    bfv.remove({ 1, 3 });
    // 12345
    // 1X345
    // 1534X
    // 153XX
    assert(bfv.at(0) == 1);
    assert(bfv.at(1) == 5);
    assert(bfv.at(2) == 3);
    assert(bfv.size() == 3);
}

void test_does_not_remove_duplicate_idx_more_than_once_variant_1()
{
    auto bfv = make_bfv_12345();
    bfv.remove({ 1, 1, 3 });
    // 12345
    // 1X345
    // 1534X
    // 153XX
    assert(bfv.at(0) == 1);
    assert(bfv.at(1) == 5);
    assert(bfv.at(2) == 3);
    assert(bfv.size() == 3);
}

void test_does_not_remove_duplicate_idx_more_than_once_variant_2()
{
    auto bfv = make_bfv_12345();
    bfv.remove({ 3, 3, 1 });
    // 12345
    // 123x5
    // 1235X
    // 1X35X
    // 153XX
    assert(bfv.at(0) == 1);
    assert(bfv.at(1) == 5);
    assert(bfv.at(2) == 3);
    assert(bfv.size() == 3);
}

void test_remove_last_item()
{
    auto bfv = make_bfv_12345();
    bfv.remove({ 4, 4 });
    assert(bfv.at(0) == 1);
    assert(bfv.at(3) == 4);
    assert(bfv.size() == 4);
}

void test_remove_first_item()
{
    auto bfv = make_bfv_12345();
    bfv.remove({ 0, 0 });
    assert(bfv.at(0) == 5);
    assert(bfv.at(1) == 2);
    assert(bfv.size() == 4);
}

void test_removes_in_backwards_order()
{
    auto bfv = make_bfv_12345();
    bfv.remove({ 4, 3 });
    assert(bfv.at(2) == 3);
    assert(bfv.size() == 3);
}

void test_removes_end_sequence()
{
    auto bfv = make_bfv_12345();
    bfv.remove({ 2, 3, 4 });
    assert(bfv.at(1) == 2);
    assert(bfv.size() == 2);
}

void test_remove_invalid_indices_does_nothing()
{
    auto bfv = make_bfv_12345();
    bfv.remove({ 5, 6 });
    assert(bfv.at(0) == 1);
    assert(bfv.at(4) == 5);
    assert(bfv.size() == 5);
}

void test_remove_empty()
{
    auto bfv = make_bfv_12345();
    bfv.remove({});
    assert(bfv.at(0) == 1);
    assert(bfv.at(4) == 5);
    assert(bfv.size() == 5);
}

void test_can_iterate_const_items()
{
    auto bfv = make_bfv_12345();
    int  x   = 0;
    for (auto const& item : bfv)
    {
        x += item;
    }
    assert(x == 15);
}


int test_backfill_vector_main()
{
    test_starts_empty();
    test_size_increases_when_items_allocated();
    test_size_decreases_when_items_removed();
    test_riterators();
    test_vector_back_fills_when_items_removed();
    test_remove_from_list_of_indices();
    test_does_not_remove_duplicate_idx_more_than_once_variant_1();
    test_does_not_remove_duplicate_idx_more_than_once_variant_2();
    test_remove_last_item();
    test_remove_first_item();
    test_removes_in_backwards_order();
    test_removes_end_sequence();
    test_remove_invalid_indices_does_nothing();
    test_can_iterate_const_items();
    printf("TEST BACKFILL complete.\n");
    return 0;
}
