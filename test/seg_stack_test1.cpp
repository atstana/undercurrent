// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <iterator>

import undercurrent;
import uc.dev;


TEST_CASE("seg_stack push") {
    uc::seg_stack<int, 4> s;
    const int n{ 10 };
    for (int i = 0; i != n; ++i) {
        s.push(i);
    }

    REQUIRE(s.size() == n);
    std::vector<int> out;
    for (int i = 0; i != n; ++i) {
        out.push_back(s[i]);
    }
    REQUIRE(out == std::vector<int>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });
}
TEST_CASE("seg_stack iterator") {
    REQUIRE(std::bidirectional_iterator<uc::seg_stack<int, 4>::iterator> == true);

    uc::seg_stack<int, 4> s;
    const int n{ 10 };
    for (int i = 0; i != n; ++i) {
        s.push(i);
    }

    std::vector<int> out;
    for (auto i : s) {
        out.push_back(i);
    }
    REQUIRE(out == std::vector<int>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });
}
TEST_CASE("seg_stack segmented_iterator advance_while") {
    REQUIRE(uc::segmented_iterator<uc::seg_stack<int, 4>::iterator> == true);
    REQUIRE(uc::segmented_iterator<uc::seg_stack<int, 4, false>::iterator> == false);

    SECTION("forward, completion, stop within last segment") {
        uc::seg_stack<int, 4> s;
        const int n{ 10 };
        for (int i = 0; i != n; ++i) {
            s.push(i);
        }

        std::vector<int> out;
        auto [pos, flag] = uc::advance_while(
            s.begin(), s.end(), [&](int i){ out.push_back(i); return true; }
        );
        REQUIRE(out == std::vector<int>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });
        REQUIRE(flag == true);
        REQUIRE(pos == s.end());
    }
    SECTION("forward, completion, stop within fake segment") {
        uc::seg_stack<int, 4> s;
        const int n{ 8 };
        for (int i = 0; i != n; ++i) {
            s.push(i);
        }

        std::vector<int> out;
        auto [pos, flag] = uc::advance_while(
            s.begin(), s.end(), [&](int i){ out.push_back(i); return true; }
        );
        REQUIRE(out == std::vector<int>{ 0, 1, 2, 3, 4, 5, 6, 7 });
        REQUIRE(flag == true);
        REQUIRE(pos == s.end());
    }
    SECTION("backward, completion") {
        uc::seg_stack<int, 4> s;
        const int n{ 10 };
        for (int i = 0; i != n; ++i) {
            s.push(i);
        }

        std::vector<int> out;
        auto [pos, flag] = uc::advance_while(
            s.begin(), s.end(), [&](int i){ out.push_back(i); return true; },
            uc::backward_tag{}
        );
        REQUIRE(out == std::vector<int>{ 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 });
        REQUIRE(flag == true);
        REQUIRE(pos == s.begin());
    }
}

TEST_CASE("seg_stack advance_while early stop") {
    SECTION("forward, stop within first segment") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };
        uc::seg_stack<int, 4> s;
        for(auto i : ints) {
            s.push(i);
        }

        auto [pos, flag] = uc::advance_while(
            s.begin(), s.end(), [](int i) { return i != 3; }
        );
        REQUIRE(flag == false);
        REQUIRE(*pos == 3);
        REQUIRE(pos == std::next(s.begin(), 2));
    }
    SECTION("forward, stop at segment boundary (first elem of next chunk)") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };
        uc::seg_stack<int, 4> s;
        for(auto i : ints) {
            s.push(i);
        }

        auto [pos, flag] = uc::advance_while(
            s.begin(), s.end(), [](int i) { return i != 5; }
        );
        REQUIRE(flag == false);
        REQUIRE(*pos == 5);
        REQUIRE(pos == std::next(s.begin(), 4));
    }
    SECTION("forward, stop within last segment") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };
        uc::seg_stack<int, 4> s;
        for(auto i : ints) {
            s.push(i);
        }

        auto [pos, flag] = uc::advance_while(
            s.begin(), s.end(), [](int i) { return i != 7; }
        );
        REQUIRE(flag == false);
        REQUIRE(*pos == 7);
        REQUIRE(pos == std::next(s.begin(), 6));
    }
    SECTION("forward, stop within middle segment (4 segments)") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };
        uc::seg_stack<int, 2> s;
        for(auto i : ints) {
            s.push(i);
        }

        auto [pos, flag] = uc::advance_while(
            s.begin(), s.end(), [](int i) { return i != 5; }
        );
        REQUIRE(flag == false);
        REQUIRE(*pos == 5);
        REQUIRE(pos == std::next(s.begin(), 4));
    }
    SECTION("backward, stop within last segment") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };
        uc::seg_stack<int, 4> s;
        for(auto i : ints) {
            s.push(i);
        }

        auto [pos, flag] = uc::advance_while(
            s.begin(), s.end(), [](int i) { return i != 6; }, uc::backward_tag{}
        );
        REQUIRE(flag == false);
        REQUIRE(*pos == 6);
        REQUIRE(pos == std::next(s.begin(), 5));
    }
    SECTION("backward, stop at setment boundary (last elem of prev seg)") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };
        uc::seg_stack<int, 4> s;
        for(auto i : ints) {
            s.push(i);
        }

        auto [pos, flag] = uc::advance_while(
            s.begin(), s.end(), [](int i) { return i != 4; }, uc::backward_tag{}
        );
        REQUIRE(flag == false);
        REQUIRE(*pos == 4);
        REQUIRE(pos == std::next(s.begin(), 3));
    }
    SECTION("backward, stop within first segment") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };
        uc::seg_stack<int, 4> s;
        for(auto i : ints) {
            s.push(i);
        }

        auto [pos, flag] = uc::advance_while(
            s.begin(), s.end(), [](int i) { return i != 2; }, uc::backward_tag{}
        );
        REQUIRE(flag == false);
        REQUIRE(*pos == 2);
        REQUIRE(pos == std::next(s.begin(), 1));
    }
    SECTION("backward, stop within middle segment (4 segments)") {
        int ints[]{ 1, 2, 3, 4, 5, 6, 7, 8 };
        uc::seg_stack<int, 2> s;
        for(auto i : ints) {
            s.push(i);
        }

        auto [pos, flag] = uc::advance_while(
            s.begin(), s.end(), [](int i) { return i != 4; }, uc::backward_tag{}
        );
        REQUIRE(flag == false);
        REQUIRE(*pos == 4);
        REQUIRE(pos == std::next(s.begin(), 3));
    }
}
