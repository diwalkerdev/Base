#pragma once

#include "dllexports.h"
#include <array>
#include <cstddef>
#include <stdexcept>
#include <vector>

#define CONTAINER (*container)

template <typename _Tp, std::size_t _Nm>
struct DLL_PUBLIC backfill_vector
{
    typedef _Tp                                   value_type;
    typedef value_type*                           pointer;
    typedef const value_type*                     const_pointer;
    typedef value_type&                           reference;
    typedef const value_type&                     const_reference;
    typedef value_type*                           iterator;
    typedef const value_type*                     const_iterator;
    typedef std::size_t                           size_type;
    typedef std::ptrdiff_t                        difference_type;
    typedef std::reverse_iterator<iterator>       reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    // Constructors.
    backfill_vector()
    {
        container = new std::array<_Tp, _Nm>;
    }

    ~backfill_vector()
    {
        delete container;
    }

    // Iterators.
    /// begin - a forward iterator starting at the first element of the container.
    constexpr auto
    begin() noexcept
    {
        return CONTAINER.begin();
    }

    constexpr auto
    begin() const noexcept
    {
        return CONTAINER.cbegin();
    }

    constexpr auto
    cbegin() const noexcept
    {
        return CONTAINER.cbegin();
    }


    /// rend - a backward iterator that points past the first element of the container.
    constexpr auto
    rend() noexcept
    {
        return CONTAINER.rend();
    }

    constexpr auto
    rend() const noexcept
    {
        return CONTAINER.crend();
    }

    constexpr auto
    crend() const noexcept
    {
        return CONTAINER.crend();
    }


    /// end - a forward iterator that points past the last element of the container.
    constexpr auto
    end() noexcept
    {
        return CONTAINER.begin() + last;
    }

    constexpr auto
    end() const noexcept
    {
        return CONTAINER.cbegin() + last;
    }

    constexpr auto
    cend() const noexcept
    {
        return CONTAINER.cbegin() + last;
    }


    /// rbegin - a backward iterator that points at the last element of the container.
    constexpr auto
    rbegin() noexcept
    {
        return CONTAINER.rbegin() + (_Nm - last);
    }

    constexpr auto
    rbegin() const noexcept
    {
        return CONTAINER.rbegin() + (_Nm - last);
    }

    constexpr auto
    crbegin() const noexcept
    {
        return CONTAINER.crbegin() + (_Nm - last);
    }


    /// back - returns the element at the back of the container.
    constexpr auto&
    back() noexcept
    {
        return *(end() - 1);
    }

    constexpr auto const&
    back() const noexcept
    {
        return *(cend() - 1);
    }

    constexpr auto const&
    cback() const noexcept
    {
        return *(cend() - 1);
    }


    // Capacity.
    constexpr size_type
    size() const noexcept { return last; }

    constexpr size_type
    capacity() const noexcept { return _Nm; }

    [[nodiscard]] constexpr bool
    empty() const noexcept { return size() == 0; }


    // Accessors.
    reference at(std::size_t pos)
    {
        if (pos < 0 || pos >= last)
        {
            throw std::out_of_range("invalid access of backfill_vector.");
        }
        return CONTAINER.at(pos);
    }

    reference operator[](size_t pos)
    {
        return CONTAINER[pos];
    }

    // Modifiers.
    /// @brief increases the container size up to a maximum of capacity.
    /// The new item can be accessed using back().
    void allocate()
    {
        auto next = last + 1;
        if (next > capacity())
        {
            throw std::out_of_range("increase exceeds size of backfill_vector.");
        }
        last = next;
    }

    void remove(size_type pos) noexcept(false)
    {
        using std::swap;
        auto& back = CONTAINER.at(last - 1);
        auto& x    = CONTAINER.at(pos);

        swap(x, back);
        last -= 1;
    }

    void remove(std::vector<std::size_t> idx)
    {
        // a is the current value to look at.
        // pivot is the position after partitioning using the value of a.
        auto                  pivot = idx.begin();
        decltype(idx.begin()) a;
        std::size_t           back = size() - 1;

        while (pivot < idx.end())
        {
            // 2 4 6 2 becomes
            // 2 2 4 6
            //
            // a p   e

            a     = pivot;
            pivot = std::partition(a, idx.end(), [a](auto const& item) { return item == *a; });

            if (*a >= size())
            {
                pivot += 1;
                continue;
            }

            while (true)
            {
                // Then check if the back is also to be removed - i.e. it is
                // not a valid swap location.
                auto count = std::count(pivot, idx.end(), back);

                if (count == 0)
                {
                    break;
                }

                back -= 1; // try a different back location.
            }

            if (back < *a)
            {
                // The item at a has been removed by reducing the back.
                // No swap is required.
                continue;
            }

            using std::swap;
            auto& item      = at(back);
            auto& to_remove = at(*a);
            swap(to_remove, item);

            back -= 1;
        }

        last = back + 1;
    }

private:
    std::array<_Tp, _Nm>* container;
    std::size_t           last{ 0 };
};

#undef CONTAINER