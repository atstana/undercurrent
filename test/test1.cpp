// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

#include <compare>
#include <iostream>
#include <iterator>
#include <ranges>
#include <catch2/catch_test_macros.hpp>

import undercurrent;

namespace sr = std::ranges;
namespace sv = std::ranges::views;

struct counter {
    int lt5{}, square{}, even{};
    void reset() {
        lt5 = 0;
        square = 0;
        even = 0;
    }

    friend bool operator==(const counter&, const counter&) = default;
    friend std::ostream& operator<<(std::ostream& os, const counter& coun) {
        os << "{ ";
        os << "lt5 : " << coun.lt5 << ", ";
        os << "square : " << coun.square << ", ";
        os << "even : " << coun.even;
        os << " }";
        return os;
    }
};

TEST_CASE("predicate call counts") {
    SECTION("take_while | transform | filter | reverse") {
        counter co;
        auto lt5 = [&](int i) { ++(co.lt5); return i < 5; };
        auto square = [&](int i) { ++(co.square); return i * i; };
        auto even = [&](int i) { ++(co.even); return i % 2 == 0; };
        std::vector<int> v{ 1, 2, 3, 4, 5, 6 };

        SECTION("std, range-based for") {
            auto r = v
                | sv::take_while(lt5)
                | sv::transform(square)
                | sv::filter(even)
                | sv::reverse;

            std::cout << "std, range-based for\n";

            for (int i : r) {};
            std::cout << "cold " << co << '\n';
            co.reset();

            for (int i : r) {}
            std::cout << "warm " << co << '\n';
            co.reset();

            std::cout << '\n';
        }
        SECTION("uc, uc::for_each") {
            auto r2 = v
                | uc::take_while(lt5)
                | uc::transform(square)
                | uc::filter(even)
                | uc::reverse;

            std::cout << "uc, uc::for_each\n";

            uc::for_each(r2.begin(), r2.end(), [](int i) {});
            REQUIRE(co == counter{ 6, 5, 5 });
            std::cout << "cold " << co << '\n';
            co.reset();

            uc::for_each(r2.begin(), r2.end(), [](int i) {});
            REQUIRE(co == counter{ 0, 3, 3 });
            std::cout << "warm " << co << '\n';
            co.reset();

            std::cout << '\n';
        }
        SECTION("uc with subrange, uc::for_each") {
            auto r3 = v
                | uc::take_while(lt5)
                | uc::transform(square)
                | uc::filter(even)
                | uc::reverse;
            
            std::cout << "uc with subrange, uc::for_each\n";

            sr::subrange s{ r3.begin(), r3.end() };
            uc::for_each(s.begin(), s.end(), [](int i) {});
            REQUIRE(co == counter{ 6, 5, 5 });
            std::cout << "cold " << co << '\n';
            co.reset();

            uc::for_each(s.begin(), s.end(), [](int i) {});
            REQUIRE(co == counter{ 0, 3, 3 });
            std::cout << "warm " << co << '\n';
            co.reset();

            std::cout << '\n';
        }
        SECTION("uc, range-based for") {
            auto r4 = v
                | uc::take_while(lt5)
                | uc::transform(square)
                | uc::filter(even)
                | uc::reverse;

            std::cout << "uc, range-based for\n";

            for (int i : r4) {};
            REQUIRE(co == counter{ 6, 10, 8 });
            std::cout << "cold " << co << '\n';
            co.reset();

            for (int i : r4) {};
            REQUIRE(co == counter{ 0, 8, 6 });
            std::cout << "warm " << co << '\n';
            co.reset();

            std::cout << '\n';
        }
        SECTION("raw loop") {
            std::cout << "raw loop\n";
            auto first = v.begin();
            auto last = [&]{
                auto cur{ first };
                while(cur != v.end() && lt5(*cur)) {
                    ++cur;
                }
                return cur;
            }();
            while(first != last) {
                --last;
                auto value{ square(*last) };
                if (even(value)) {}
            }

            REQUIRE(co == counter{ 5, 4, 4 });
            std::cout << co << '\n';
            co.reset();

            std::cout << '\n';
        }
    }
}

TEST_CASE("join_view advance_while") {
    SECTION("reference inner ranges, with empty inner ranges") {
        std::vector<std::vector<int>> vv{ {1, 2}, {}, {3}, {}, {4, 5, 6} };

        SECTION("forward, completion: flatten the whole range") {
            auto j = vv | uc::join;
            std::vector<int> out;
            uc::for_each(j.begin(), j.end(), [&](int i) { out.push_back(i); });
            REQUIRE(out == std::vector<int>{ 1, 2, 3, 4, 5, 6 });
        }
        SECTION("forward, early stop: find_if stops at the first match") {
            auto j = vv | uc::join;
            auto it = uc::find_if(
                j.begin(), j.end(), [](int i) { return i == 4; }
            );
            REQUIRE(it != j.end());
            REQUIRE(*it == 4);
        }
        SECTION("forward, no match: find_if runs to the end") {
            auto j = vv | uc::join;
            auto it = uc::find_if(
                j.begin(), j.end(), [](int i) { return i > 100; }
            );
            REQUIRE(it == j.end());
        }
        SECTION("backward, completion: scanning the whole range returns begin") {
            auto j = vv | uc::join;
            auto res = uc::advance_while(
                j.begin(), j.end(),
                [](int) { return true; },
                uc::backward_tag{}
            );
            REQUIRE(res.completion_flag == true);
            REQUIRE(res.pos == j.begin());
        }
        SECTION("backward, early stop: visiting 6, 5, 4 then stopping at 3") {
            auto j = vv | uc::join;
            auto res = uc::advance_while(
                j.begin(), j.end(),
                [](int i) { return i != 3; },
                uc::backward_tag{}
            );
            REQUIRE(res.completion_flag == false);
            REQUIRE(*res.pos == 3);
        }
    }

    SECTION("non-reference inner ranges (sentinel path), with empty inner range") {
        std::vector<int> ns{ 2, 0, 3 };
        auto j = ns
            | uc::transform([](int n) { return std::views::iota(0, n); })
            | uc::join;

        std::vector<int> out;
        uc::for_each(j.begin(), j.end(), [&](int i) { out.push_back(i); });
        REQUIRE(out == std::vector<int>{ 0, 1, 0, 1, 2 });
    }
}

TEST_CASE("reverse_iterator find_if") {
    std::vector<int> v{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };

    SECTION("iter concept test") {
        auto r = v | uc::reverse;
        REQUIRE(std::random_access_iterator<decltype(r.begin())> == true);
    }
    SECTION("simple case") {
        auto r = v | uc::reverse;
        auto it = uc::find_if(r.begin(), r.end(), [](int i){ return i == 8; });
        REQUIRE(*it == 8);
    }
    SECTION("filter | reverse") {
        auto mod2 = [](int i) { return i % 2 == 0; };

        auto r = v | uc::filter(mod2) | uc::reverse;
        auto it = uc::find_if(r.begin(), r.end(), [](int i){ return i == 8; });
        REQUIRE(*it == 8);
    }
    SECTION("filter | filter | reverse") {
        auto mod2 = [](int i) { return i % 2 == 0; };
        auto mod4 = [](int i) { return i % 4 == 0; };

        auto r = v | uc::filter(mod2) | uc::filter(mod4) | uc::reverse;
        auto it = uc::find_if(r.begin(), r.end(), [](int i){ return i == 8; });
        REQUIRE(*it == 8);
    }
}