#pragma once

#include "dllexports.h"
#include <cassert>
#include <cstddef>

#define CONTAINER (container)

template <typename _Tp, size_t Nm>
struct Array
{
    typedef _Tp               value_type;
    typedef value_type*       pointer;
    typedef const value_type* const_pointer;
    typedef value_type&       reference;
    typedef const value_type& const_reference;
    typedef value_type*       iterator;
    typedef const value_type* const_iterator;
    typedef ptrdiff_t         difference_type;

    // Data members
    //
    value_type  container[Nm];
    size_t last{ 0 };

    // Modifiers
    //
    bool reserve(size_t n)
    {
        auto size = last + n;
        if (size > Nm)
        {
            return false;
        }

        last = size;
        return true;
    }

    // Iterators.
    /// begin - a forward iterator starting at the first element of the container.
    constexpr auto
    begin() noexcept
    {
        return iterator(&container);
    }

    constexpr auto
    begin() const noexcept
    {
        return const_iterator(&container);
    }

    constexpr auto
    cbegin() const noexcept
    {
        return const_iterator(&container);
    }


    /// rend - a backward iterator that points past the first element of the container.
    // constexpr auto
    // rend() noexcept
    // {
    //     return CONTAINER.rend();
    // }

    // constexpr auto
    // rend() const noexcept
    // {
    //     return CONTAINER.crend();
    // }

    // constexpr auto
    // crend() const noexcept
    // {
    //     return CONTAINER.crend();
    // }


    /// end - a forward iterator that points past the last element of the container.
    constexpr auto
    end() noexcept
    {
        return iterator(&container[last]);
    }

    constexpr auto
    end() const noexcept
    {
        return const_iterator(&container[last]);
    }

    constexpr auto
    cend() const noexcept
    {
        return const_iterator(&container[last]);
    }


    /// rbegin - a backward iterator that points at the last element of the container.
    // constexpr auto
    // rbegin() noexcept
    // {
    //     return CONTAINER.rbegin() + (Nm - last);
    // }

    // constexpr auto
    // rbegin() const noexcept
    // {
    //     return CONTAINER.rbegin() + (Nm - last);
    // }

    // constexpr auto
    // crbegin() const noexcept
    // {
    //     return CONTAINER.crbegin() + (Nm - last);
    // }


    // Capacity Functions.
    //
    constexpr size_t
    size() const noexcept { return last; }

    constexpr size_t
    capacity() const noexcept { return Nm; }

    constexpr bool
    empty() const noexcept { return size() == 0; }

    constexpr bool
    full() const noexcept { return size() == Nm; }


    // Access Functions.
    //
    reference
    operator[](size_t pos)
    {
        assert(pos < Nm);
        return CONTAINER[pos];
    }

    /// back - returns the element at the back of the container.
    constexpr auto&
    back() noexcept
    {
        return CONTAINER[last - 1];
    }

    constexpr auto&
    back() const noexcept
    {
        return CONTAINER[last - 1];
    }

    constexpr auto&
    cback() const noexcept
    {
        return CONTAINER[last - 1];
    }
};

template <typename Tp, size_t Size>
bool
Array_Reserve(Array<Tp, Size>& array, size_t n = 1)
{
    auto size = array.last + n;
    if (size > Size)
    {
        return false;
    }

    array.last = size;
    return true;
}

template <typename Tp, size_t Size>
constexpr size_t
Array_Size(Array<Tp, Size> const& array) noexcept
{
    return array.last;
}

template <typename Tp, size_t Size>
constexpr size_t
Array_Capacity(Array<Tp, Size> const&) noexcept
{
    return Size;
}

template <typename Tp, size_t Size>
constexpr bool
Array_Empty(Array<Tp, Size> const& array) noexcept
{
    return array.last == 0;
}

template <typename Tp, size_t Size>
constexpr bool
Array_Full(Array<Tp, Size> const& array) noexcept
{
    return array.last == Size;
}

#undef CONTAINER