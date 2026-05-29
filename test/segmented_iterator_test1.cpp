// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <compare>
#include <iostream>
#include <iterator>
#include <ranges>
#include <catch2/catch_test_macros.hpp>

import undercurrent;
import uc.dev;

namespace sr = std::ranges;
namespace sv = std::ranges::views;

TEST_CASE("chunked_iterator advance_while") {
    SECTION("simple1") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };

        uc::chunked_iterator<int, 4> first{ ints };
        uc::chunked_iterator<int, 4> last{ ints, 8, 8 };
        std::vector<int> out;
        uc::for_each(first, last, [&](int i){ out.push_back(i);});
        REQUIRE(out == std::vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8 });
    }
    SECTION("simple2") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };

        uc::chunked_iterator<int, 3> first{ ints };
        uc::chunked_iterator<int, 3> last{ ints, 8, 8 };
        std::vector<int> out;
        uc::for_each(first, last, [&](int i){ out.push_back(i); });
        REQUIRE(out == std::vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8 });
    }
    SECTION("reverse simple1") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };

        uc::chunked_iterator<int, 4> first{ ints };
        uc::chunked_iterator<int, 4> last{ ints, 8, 8 };
        auto r_first = uc::make_iterator_reversed(last);
        auto r_last = uc::make_iterator_reversed(first);

        std::vector<int> out;
        uc::for_each(r_first, r_last, [&](int i){ out.push_back(i); });
        REQUIRE(out == std::vector<int>{ 8, 7, 6, 5, 4, 3, 2, 1 });
    }
    SECTION("reverse simple2") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };

        uc::chunked_iterator<int, 3> first{ ints };
        uc::chunked_iterator<int, 3> last{ ints, 8, 8 };
        auto r_first = uc::make_iterator_reversed(last);
        auto r_last = uc::make_iterator_reversed(first);

        std::vector<int> out;
        uc::for_each(r_first, r_last, [&](int i){ out.push_back(i); });
        REQUIRE(out == std::vector<int>{ 8, 7, 6, 5, 4, 3, 2, 1 });
    }
}

TEST_CASE("chunked_iterator advance_while early stop") {
    SECTION("forward, stop within first segment") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };
        uc::chunked_iterator<int, 4> first{ ints };
        uc::chunked_iterator<int, 4> last { ints, 8, 8 };

        auto [pos, flag] = uc::advance_while(
            first, last, [](int i) { return i != 3; }
        );
        REQUIRE(flag == false);
        REQUIRE(*pos == 3);
        REQUIRE(pos == uc::chunked_iterator<int, 4>{ ints, 8, 2 });
    }
    SECTION("forward, stop at chunk boundary (first elem of next chunk)") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };
        uc::chunked_iterator<int, 4> first{ ints };
        uc::chunked_iterator<int, 4> last { ints, 8, 8 };

        auto [pos, flag] = uc::advance_while(
            first, last, [](int i) { return i != 5; }
        );
        REQUIRE(flag == false);
        REQUIRE(*pos == 5);
        REQUIRE(pos == uc::chunked_iterator<int, 4>{ ints, 8, 4 });
    }
    SECTION("forward, stop within last segment") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };
        uc::chunked_iterator<int, 4> first{ ints };
        uc::chunked_iterator<int, 4> last { ints, 8, 8 };

        auto [pos, flag] = uc::advance_while(
            first, last, [](int i) { return i != 7; }
        );
        REQUIRE(flag == false);
        REQUIRE(*pos == 7);
        REQUIRE(pos == uc::chunked_iterator<int, 4>{ ints, 8, 6 });
    }
    SECTION("forward, stop within middle segment (4 chunks)") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };
        uc::chunked_iterator<int, 2> first{ ints };
        uc::chunked_iterator<int, 2> last { ints, 8, 8 };

        auto [pos, flag] = uc::advance_while(
            first, last, [](int i) { return i != 5; }
        );
        REQUIRE(flag == false);
        REQUIRE(*pos == 5);
        REQUIRE(pos == uc::chunked_iterator<int, 2>{ ints, 8, 4 });
    }
    SECTION("backward, stop within last segment") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };
        uc::chunked_iterator<int, 4> first{ ints };
        uc::chunked_iterator<int, 4> last { ints, 8, 8 };

        auto [pos, flag] = uc::advance_while(
            first, last, [](int i) { return i != 6; }, uc::backward_tag{}
        );
        REQUIRE(flag == false);
        REQUIRE(*pos == 6);
        REQUIRE(pos == uc::chunked_iterator<int, 4>{ ints, 8, 5 });
    }
    SECTION("backward, stop at chunk boundary (last elem of prev chunk)") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };
        uc::chunked_iterator<int, 4> first{ ints };
        uc::chunked_iterator<int, 4> last { ints, 8, 8 };

        auto [pos, flag] = uc::advance_while(
            first, last, [](int i) { return i != 4; }, uc::backward_tag{}
        );
        REQUIRE(flag == false);
        REQUIRE(*pos == 4);
        REQUIRE(pos == uc::chunked_iterator<int, 4>{ ints, 8, 3 });
    }
    SECTION("backward, stop within first segment") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };
        uc::chunked_iterator<int, 4> first{ ints };
        uc::chunked_iterator<int, 4> last { ints, 8, 8 };

        auto [pos, flag] = uc::advance_while(
            first, last, [](int i) { return i != 2; }, uc::backward_tag{}
        );
        REQUIRE(flag == false);
        REQUIRE(*pos == 2);
        REQUIRE(pos == uc::chunked_iterator<int, 4>{ ints, 8, 1 });
    }
    SECTION("backward, stop within middle segment (4 chunks)") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };
        uc::chunked_iterator<int, 2> first{ ints };
        uc::chunked_iterator<int, 2> last { ints, 8, 8 };

        auto [pos, flag] = uc::advance_while(
            first, last, [](int i) { return i != 4; }, uc::backward_tag{}
        );
        REQUIRE(flag == false);
        REQUIRE(*pos == 4);
        REQUIRE(pos == uc::chunked_iterator<int, 2>{ ints, 8, 3 });
    }
}

TEST_CASE("chunked_iterator advance_while same segment") {
    SECTION("forward, completion") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };
        uc::chunked_iterator<int, 4> first{ ints, 8, 1 };
        uc::chunked_iterator<int, 4> last { ints, 8, 3 };

        std::vector<int> out;
        uc::for_each(first, last, [&](int i) { out.push_back(i); });
        REQUIRE(out == std::vector<int>{ 2, 3 });
    }
    SECTION("forward, early stop") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };
        uc::chunked_iterator<int, 4> first{ ints, 8, 1 };
        uc::chunked_iterator<int, 4> last { ints, 8, 3 };

        auto [pos, flag] = uc::advance_while(
            first, last, [](int i) { return i != 3; }
        );
        REQUIRE(flag == false);
        REQUIRE(*pos == 3);
        REQUIRE(pos == uc::chunked_iterator<int, 4>{ ints, 8, 2 });
    }
    SECTION("backward, completion") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };
        uc::chunked_iterator<int, 4> first{ ints, 8, 1 };
        uc::chunked_iterator<int, 4> last { ints, 8, 3 };

        auto [pos, flag] = uc::advance_while(
            first, last, [](int){ return true; }, uc::backward_tag{}
        );
        REQUIRE(flag == true);
        REQUIRE(pos == first);
    }
    SECTION("backward, early stop") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };
        uc::chunked_iterator<int, 4> first{ ints, 8, 1 };
        uc::chunked_iterator<int, 4> last { ints, 8, 3 };

        auto [pos, flag] = uc::advance_while(
            first, last, [](int i) { return i != 2; }, uc::backward_tag{}
        );
        REQUIRE(flag == false);
        REQUIRE(*pos == 2);
        REQUIRE(pos == uc::chunked_iterator<int, 4>{ ints, 8, 1 });
    }
}

TEST_CASE("chunked_iterator advance_while empty range") {
    SECTION("forward, at chunk boundary") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };
        uc::chunked_iterator<int, 4> it{ ints, 8, 4 };

        int count = 0;
        auto [pos, flag] = uc::advance_while(
            it, it, [&](int){ ++count; return true; }
        );
        REQUIRE(flag == true);
        REQUIRE(pos == it);
        REQUIRE(count == 0);
    }
    SECTION("forward, mid chunk") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };
        uc::chunked_iterator<int, 4> it{ ints, 8, 2 };

        int count = 0;
        auto [pos, flag] = uc::advance_while(
            it, it, [&](int){ ++count; return true; }
        );
        REQUIRE(flag == true);
        REQUIRE(pos == it);
        REQUIRE(count == 0);
    }
    SECTION("forward, at past-the-end") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };
        uc::chunked_iterator<int, 4> it{ ints, 8, 8 };

        int count = 0;
        auto [pos, flag] = uc::advance_while(
            it, it, [&](int){ ++count; return true; }
        );
        REQUIRE(flag == true);
        REQUIRE(pos == it);
        REQUIRE(count == 0);
    }
    SECTION("backward, mid chunk") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };
        uc::chunked_iterator<int, 4> it{ ints, 8, 2 };

        int count = 0;
        auto [pos, flag] = uc::advance_while(
            it, it, [&](int){ ++count; return true; }, uc::backward_tag{}
        );
        REQUIRE(flag == true);
        REQUIRE(pos == it);
        REQUIRE(count == 0);
    }
}
