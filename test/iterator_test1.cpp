// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

#include <concepts>
#include <deque>
#include <forward_list>
#include <functional>
#include <istream>
#include <iterator>
#include <memory>
#include <list>
#include <ranges>
#include <string>
#include <type_traits>
#include <vector>
#include <catch2/catch_test_macros.hpp>

import undercurrent;

namespace sr = std::ranges;

using list_it = std::list<int>::iterator;
using deque_it = std::deque<int>::iterator;
using flist_it = std::forward_list<int>::iterator;

// calc_iter_concept must not cap the category below the iterator's own one.
static_assert(std::same_as<
    uc::reverse_iterator<list_it>::iterator_concept,
    std::bidirectional_iterator_tag
>);
static_assert(std::bidirectional_iterator<uc::reverse_iterator<list_it>>);

static_assert(std::same_as<
    uc::basic_const_iterator<deque_it>::iterator_concept,
    std::random_access_iterator_tag
>);
static_assert(std::random_access_iterator<uc::basic_const_iterator<deque_it>>);

static_assert(std::same_as<
    uc::basic_const_iterator<flist_it>::iterator_concept,
    std::forward_iterator_tag
>);
static_assert(std::forward_iterator<uc::basic_const_iterator<flist_it>>);

static_assert(std::random_access_iterator<uc::basic_const_iterator<int*>>);
static_assert(std::sized_sentinel_for<
    uc::basic_const_iterator<int*>, uc::basic_const_iterator<int*>
>);

// basic_const_iterator is contiguous when the base iterator is.
static_assert(std::contiguous_iterator<uc::basic_const_iterator<int*>>);
static_assert(std::contiguous_iterator<
    uc::basic_const_iterator<std::vector<int>::iterator>
>);

// basic_const_iterator only offers what the base iterator supports.
template<typename T>
concept decrementable = requires(T& t) { --t; t--; };
static_assert(!decrementable<uc::basic_const_iterator<flist_it>>);
static_assert(decrementable<uc::basic_const_iterator<list_it>>);

static_assert(std::same_as<
    decltype(std::declval<
        uc::basic_const_iterator<std::istream_iterator<int>>&
    >()++),
    void
>);

// An iterator with its own iter_move / iter_swap, to check that the
// adaptors forward to them instead of falling back to std::move(*it).
struct tracked_iter {
    using value_type = int;
    using difference_type = std::ptrdiff_t;

    inline static int iter_move_calls = 0;
    inline static int iter_swap_calls = 0;

    int* p{};

    int& operator*() const { return *p; }
    tracked_iter& operator++() { ++p; return *this; }
    tracked_iter operator++(int) { auto t{ *this }; ++p; return t; }
    tracked_iter& operator--() { --p; return *this; }
    tracked_iter operator--(int) { auto t{ *this }; --p; return t; }
    friend bool operator==(const tracked_iter&, const tracked_iter&) = default;

    friend int&& iter_move(const tracked_iter& i) {
        ++iter_move_calls;
        return std::move(*i.p);
    }
    friend void iter_swap(const tracked_iter& x, const tracked_iter& y) {
        ++iter_swap_calls;
        std::swap(*x.p, *y.p);
    }
};
static_assert(std::bidirectional_iterator<tracked_iter>);

static_assert(std::same_as<
    std::iter_rvalue_reference_t<uc::basic_const_iterator<tracked_iter>>,
    const int&&
>);

// reverse_iterator converting constructor.
static_assert(std::is_convertible_v<
    uc::reverse_iterator<int*>, uc::reverse_iterator<const int*>
>);
static_assert(!std::is_constructible_v<
    uc::reverse_iterator<int*>, uc::reverse_iterator<const int*>
>);

// skip_t accepts any arguments, including class types.
static_assert(std::invocable<uc::skip_t, std::string&>);
static_assert(std::invocable<uc::skip_t, std::string&&>);
static_assert(uc::skip(1, 2));

// next is usable in constant expressions.
constexpr int next_in_constexpr() {
    int a[]{ 1, 2, 3 };
    auto e = uc::next(a + 0, a + 3);
    auto ce = uc::next(
        uc::basic_const_iterator<int*>{ a + 0 },
        uc::basic_const_iterator<int*>{ a + 3 }
    );
    return (e == a + 3 && ce.base() == a + 3) ? *(e - 1) : -1;
}
static_assert(next_in_constexpr() == 3);

TEST_CASE("basic_const_iterator") {
    int arr[]{ 10, 20, 30, 40 };
    int* a = arr;    // avoid deducing array types in the operator templates
    using bci = uc::basic_const_iterator<int*>;
    bci it{ a + 1 };

    SECTION("n + it") {
        CHECK(*(2 + it) == 40);
        CHECK((2 + it) == (it + 2));
    }

    SECTION("comparisons with the underlying iterator on the left") {
        CHECK(a < it);
        CHECK(a + 1 <= it);
        CHECK(a + 3 > it);
        CHECK(a + 1 >= it);
        CHECK(!(a + 2 < it));
    }

    SECTION("comparisons with the underlying iterator on the right") {
        CHECK(it > a);
        CHECK(it >= a + 1);
        CHECK(it < a + 3);
        CHECK(it <= a + 1);
    }

    SECTION("difference with the underlying iterator") {
        CHECK((a + 3) - it == 2);
        CHECK(it - a == 1);
        CHECK(bci{ a + 3 } - it == 2);
    }

    SECTION("operator-> and to_address") {
        CHECK(std::to_address(it) == a + 1);
        CHECK(it.operator->() == a + 1);

        std::list<std::string> l{ "abc" };
        uc::basic_const_iterator<std::list<std::string>::iterator> li{ l.begin() };
        CHECK(li->size() == 3);
    }

    SECTION("iter_move forwards to the base iterator") {
        tracked_iter::iter_move_calls = 0;
        uc::basic_const_iterator<tracked_iter> ti{ tracked_iter{ a + 2 } };
        int x = std::ranges::iter_move(ti);
        CHECK(x == 30);
        CHECK(tracked_iter::iter_move_calls == 1);
    }

    SECTION("next and find_if go through advance_while") {
        bci first{ a + 0 }, last{ a + 4 };
        CHECK(uc::next(first, last) == last);
        auto pos = uc::find_if(first, last, [](int i) { return i == 30; });
        CHECK(pos.base() == a + 2);
    }
}

TEST_CASE("reverse_iterator") {
    SECTION("make_reverse_iterator accepts lvalues") {
        int a[]{ 1, 2, 3 };
        int* p = a + 3;
        auto r = uc::make_reverse_iterator(p);
        static_assert(std::same_as<decltype(r), uc::reverse_iterator<int*>>);
        CHECK(*r == 3);

        std::list<int> l{ 1, 2, 3 };
        auto e = l.end();
        auto rl = uc::make_reverse_iterator(e);
        static_assert(std::same_as<decltype(rl), uc::reverse_iterator<list_it>>);
        CHECK(*rl == 3);
    }

    SECTION("converting constructor") {
        int a[]{ 1, 2, 3 };
        uc::reverse_iterator<int*> r{ a + 3 };
        uc::reverse_iterator<const int*> cr = r;
        CHECK(cr.base() == a + 3);
        CHECK(*cr == 3);
    }

    SECTION("operator->") {
        std::string s[]{ "a", "bb", "ccc" };
        uc::reverse_iterator<std::string*> rp{ s + 3 };
        CHECK(rp->size() == 3);

        std::list<std::string> l{ "a", "bb" };
        uc::reverse_iterator<std::list<std::string>::iterator> rl{ l.end() };
        CHECK(rl->size() == 2);
    }

    SECTION("iter_move and iter_swap forward to the base iterator") {
        int a[]{ 1, 2, 3 };
        tracked_iter::iter_move_calls = 0;
        tracked_iter::iter_swap_calls = 0;

        uc::reverse_iterator<tracked_iter> r1{ tracked_iter{ a + 3 } };
        uc::reverse_iterator<tracked_iter> r2{ tracked_iter{ a + 1 } };

        int x = std::ranges::iter_move(r1);
        CHECK(x == 3);
        CHECK(tracked_iter::iter_move_calls == 1);

        std::ranges::iter_swap(r1, r2);
        CHECK(a[0] == 3);
        CHECK(a[2] == 1);
        CHECK(tracked_iter::iter_swap_calls == 1);
    }

    SECTION("filter | reverse stays bidirectional") {
        std::vector<int> v{ 1, 2, 3, 4, 5, 6 };
        auto r = v | uc::filter([](int i) { return i % 2 == 0; }) | uc::reverse;
        static_assert(sr::bidirectional_range<decltype(r)>);
        auto last = r.end();
        --last;
        CHECK(*last == 2);
    }
}
