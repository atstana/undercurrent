// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

#include <concepts>
#include <cstddef>
#include <iterator>
#include <memory>
#include <ranges>
#include <sstream>
#include <string>
#include <vector>
#include <catch2/catch_test_macros.hpp>

import undercurrent;

namespace sr = std::ranges;
namespace sv = std::ranges::views;

template<typename T>
concept decrementable = requires(T& t) { --t; };

// join_view::iterator::operator-- needs a common inner range.
using unbounded = sr::subrange<int*, std::unreachable_sentinel_t>;
using join_of_unbounded = decltype(
    std::declval<std::vector<unbounded>&>() | uc::join
);
static_assert(!decrementable<sr::iterator_t<join_of_unbounded>>);

TEST_CASE("transform_view") {
    SECTION("random access arithmetic") {
        std::vector<int> v{ 1, 2, 3, 4 };
        auto r = v | uc::transform([](int i) { return i * 10; });
        auto it = r.begin();
        CHECK(*(it + 2) == 30);
        CHECK(*(2 + it) == 30);
        CHECK(*((it + 3) - 1) == 30);
    }

    SECTION("reference is computed with an lvalue function object") {
        struct lvalue_only {
            int operator()(int i) const& { return i + 1; }
            void operator()(int) && = delete;
        };
        std::vector<int> v{ 1, 2, 3 };
        auto r = v | uc::transform(lvalue_only{});
        static_assert(std::same_as<sr::range_reference_t<decltype(r)>, int>);
        int sum{};
        uc::for_each(r.begin(), r.end(), [&](int i) { sum += i; });
        CHECK(sum == 9);
    }

    SECTION("advance_while is customized even if the element type changes") {
        // reverse over filter is expensive without the customization, so
        // the predicate call count tells which path was taken.
        std::vector<int> v{ 1, 2, 3, 4, 5, 6 };
        int calls{};
        auto even = [&](int i) { ++calls; return i % 2 == 0; };

        auto same_type = v | uc::filter(even) | uc::reverse
            | uc::transform([](int i) { return i; });
        uc::for_each(same_type.begin(), same_type.end(), [](int) {});
        const int expected = calls;

        calls = 0;
        auto other_type = v | uc::filter(even) | uc::reverse
            | uc::transform([](int i) { return std::to_string(i); });
        std::string joined;
        uc::for_each(
            other_type.begin(), other_type.end(),
            [&](const std::string& s) { joined += s; }
        );
        CHECK(joined == "642");
        CHECK(calls == expected);
    }

    SECTION("a move-only function object makes the view move-only") {
        std::vector<int> v{ 1, 2, 3 };
        auto r = v | uc::transform(
            [p = std::make_unique<int>(1)](int i) { return i + *p; }
        );
        static_assert(std::movable<decltype(r)>);
        static_assert(!std::copy_constructible<decltype(r)>);
        int sum{};
        uc::for_each(r.begin(), r.end(), [&](int i) { sum += i; });
        CHECK(sum == 9);
    }
}

TEST_CASE("rvalue-reference elements are forwarded through composed predicates") {
    std::vector<std::string> v{ "a", "bb", "ccc" };
    sr::subrange mv{
        std::make_move_iterator(v.begin()),
        std::make_move_iterator(v.end())
    };
    auto size_of = [](std::string&& s) { return s.size(); };

    SECTION("filter | transform") {
        auto r = mv
            | uc::filter([](const std::string& s) { return s.size() != 2; })
            | uc::transform(size_of);
        std::size_t sum{};
        uc::for_each(r.begin(), r.end(), [&](std::size_t n) { sum += n; });
        CHECK(sum == 4);
    }

    SECTION("take_while | transform") {
        auto r = mv
            | uc::take_while([](const std::string& s) { return s.size() < 3; })
            | uc::transform(size_of);
        std::size_t sum{};
        uc::for_each(r.begin(), r.end(), [&](std::size_t n) { sum += n; });
        CHECK(sum == 3);
    }
}

TEST_CASE("drop_while_view over an input range with move-only iterators") {
    std::istringstream ss{ "1 2 3 4 5" };
    auto r = sv::istream<int>(ss) | uc::drop_while([](int i) { return i < 3; });
    int sum{};
    for (int i : r) {
        sum += i;
    }
    CHECK(sum == 12);
}

TEST_CASE("join_view of a simple view with prvalue inner ranges") {
    auto t = sv::iota(0, 3)
        | sv::transform([](int i) { return std::vector<int>(i, i); });
    sr::subrange s{ t.begin(), t.end() };
    auto j = s | uc::join;
    using J = decltype(j);
    static_assert(sr::input_range<J>);
    // begin() and end() must agree on constness; otherwise the sentinel's
    // advance_while customization is bypassed through an implicit
    // iterator<false> -> iterator<true> conversion.
    static_assert(std::same_as<sr::iterator_t<J>, J::iterator<false>>);
    static_assert(std::same_as<sr::sentinel_t<J>, J::sentinel<false>>);

    int sum{};
    uc::for_each(j.begin(), j.end(), [&](int i) { sum += i; });
    CHECK(sum == 5);

    sum = 0;
    for (int i : j) {
        sum += i;
    }
    CHECK(sum == 5);
}
