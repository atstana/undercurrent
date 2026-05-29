// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <ranges>

import undercurrent;
import uc.dev;

namespace sr = std::ranges;
namespace sv = std::ranges::views;

template<int N>
struct lt {
    bool operator()(int i) const { return i < N; };
};
template<int N>
struct mul {
    int operator()(int i) const { return i * N; };
};
struct square {
    int operator()(int i) const { return i * i; };
};
template<int N>
struct mod {
    bool operator()(int i) const { return i % N == 0; };
};

double std_simple() {
    constexpr int n = 2000;
    auto v = uc::vrand_unique(0, n);
    uc::seg_stack<int, 128> s;
    for(auto i : v) {
        s.push(i);
    }
    auto start = std::chrono::steady_clock::now();
    int sum{};
    std::ranges::for_each(s.begin(), s.end(), [&](int i){ sum += i; });
    auto end = std::chrono::steady_clock::now();

    if (sum == 0) {
        std::cout << sum << '\n';
    }
    return std::chrono::duration<double, std::micro>{ end - start }.count();
}
double std_cold() {
    constexpr int n = 2000;
    auto v = uc::vrand_unique(0, n);
    uc::seg_stack<int, 128> s;
    for(auto i : v) {
        s.push(i);
    }

    auto r = s
        | sv::take_while(lt<n>{})
        | sv::transform(square{})
        | sv::filter(mod<2>{})
        | sv::reverse;

    auto start = std::chrono::steady_clock::now();
    int sum{};
    std::ranges::for_each(r.begin(), r.end(), [&](int i){ sum += i; });
    auto end = std::chrono::steady_clock::now();

    if (sum == 0) {
        std::cout << sum << '\n';
    }
    return std::chrono::duration<double, std::micro>{ end - start }.count();
}
double std_warm() {
    constexpr int n = 2000;
    auto v = uc::vrand_unique(0, n);
    uc::seg_stack<int, 128> s;
    for(auto i : v) {
        s.push(i);
    }

    auto r = s
        | sv::take_while(lt<n>{})
        | sv::transform(square{})
        | sv::filter(mod<2>{})
        | sv::reverse;

    (void)r.begin();
    auto start = std::chrono::steady_clock::now();
    int sum{};
    std::ranges::for_each(r.begin(), r.end(), [&](int i){ sum += i; });
    auto end = std::chrono::steady_clock::now();

    if (sum == 0) {
        std::cout << sum << '\n';
    }
    return std::chrono::duration<double, std::micro>{ end - start }.count();
}

double uc_seg_simple() {
    constexpr int n = 2000;
    auto v = uc::vrand_unique(0, n);
    uc::seg_stack<int, 128> s;
    for(auto i : v) {
        s.push(i);
    }
    auto start = std::chrono::steady_clock::now();
    int sum{};
    uc::for_each(s.begin(), s.end(), [&](int i) { sum += i; });
    auto end = std::chrono::steady_clock::now();

    if (sum == 0) {
        std::cout << sum << '\n';
    }
    return std::chrono::duration<double, std::micro>{ end - start }.count();
}
double uc_seg_cold() {
    constexpr int n = 2000;
    auto v = uc::vrand_unique(0, n);
    uc::seg_stack<int, 128> s;
    for(auto i : v) {
        s.push(i);
    }

    auto r = s
        | uc::take_while(lt<n>{})
        | uc::transform(square{})
        | uc::filter(mod<2>{})
        | uc::reverse;

    auto start = std::chrono::steady_clock::now();
    int sum{};
    uc::for_each(r.begin(), r.end(), [&](int i) { sum += i; });
    auto end = std::chrono::steady_clock::now();

    if (sum == 0) {
        std::cout << sum << '\n';
    }
    return std::chrono::duration<double, std::micro>{ end - start }.count();
}
double uc_seg_warm() {
    constexpr int n = 2000;
    auto v = uc::vrand_unique(0, n);
    uc::seg_stack<int, 128> s;
    for(auto i : v) {
        s.push(i);
    }

    auto r = s
        | uc::take_while(lt<n>{})
        | uc::transform(square{})
        | uc::filter(mod<2>{})
        | uc::reverse;

    (void)r.begin();
    auto start = std::chrono::steady_clock::now();
    int sum{};
    uc::for_each(r.begin(), r.end(), [&](int i) { sum += i; });
    auto end = std::chrono::steady_clock::now();

    if (sum == 0) {
        std::cout << sum << '\n';
    }
    return std::chrono::duration<double, std::micro>{ end - start }.count();
}

double uc_simple() {
    constexpr int n = 2000;
    auto v = uc::vrand_unique(0, n);
    uc::seg_stack<int, 128, false> s;
    for(auto i : v) {
        s.push(i);
    }
    auto start = std::chrono::steady_clock::now();
    int sum{};
    uc::for_each(s.begin(), s.end(), [&](int i) { sum += i; });
    auto end = std::chrono::steady_clock::now();

    if (sum == 0) {
        std::cout << sum << '\n';
    }
    return std::chrono::duration<double, std::micro>{ end - start }.count();
}
double uc_cold() {
    constexpr int n = 2000;
    auto v = uc::vrand_unique(0, n);
    uc::seg_stack<int, 128, false> s;
    for(auto i : v) {
        s.push(i);
    }

    auto r = s
        | uc::take_while(lt<n>{})
        | uc::transform(square{})
        | uc::filter(mod<2>{})
        | uc::reverse;

    auto start = std::chrono::steady_clock::now();
    int sum{};
    uc::for_each(r.begin(), r.end(), [&](int i) { sum += i; });
    auto end = std::chrono::steady_clock::now();

    if (sum == 0) {
        std::cout << sum << '\n';
    }
    return std::chrono::duration<double, std::micro>{ end - start }.count();
}
double uc_warm() {
    constexpr int n = 2000;
    auto v = uc::vrand_unique(0, n);
    uc::seg_stack<int, 128, false> s;
    for(auto i : v) {
        s.push(i);
    }

    auto r = s
        | uc::take_while(lt<n>{})
        | uc::transform(square{})
        | uc::filter(mod<2>{})
        | uc::reverse;

    (void)r.begin();
    auto start = std::chrono::steady_clock::now();
    int sum{};
    uc::for_each(r.begin(), r.end(), [&](int i) { sum += i; });
    auto end = std::chrono::steady_clock::now();

    if (sum == 0) {
        std::cout << sum << '\n';
    }
    return std::chrono::duration<double, std::micro>{ end - start }.count();
}

//double raw_loop() {
//    constexpr int n = 2000;
//    auto v = uc::vrand_unique(0, n);
//
//    auto ltn = lt<n>{};
//    auto sq = square{};
//    auto modn = mod<2>{};
//
//    int sum{};
//    auto start = std::chrono::steady_clock::now();
//    auto first{ v.begin() };
//    auto last = [&] {
//        auto cur{ first };
//        while((cur != v.end()) && std::invoke(ltn, *cur)) {
//            ++cur;
//        }
//        return cur;
//    }();
//    while(first != last) {
//        --last;
//        const auto value{ std::invoke(sq, *last) };
//        if (modn(value)) {
//            sum += value;
//        }
//    }
//    auto end = std::chrono::steady_clock::now();
//
//    if (sum == 0) {
//        std::cout << sum << '\n';
//    }
//    return std::chrono::duration<double, std::micro>{ end - start }.count();
//}

int main() {
    {
        const int n{ 20 };
        std::vector<double> samples;
        std::cout << "std simple\n";
        for (int i = 0; i != n; ++i) {
            samples.push_back(std_simple());
        }
        std::cout << uc::get_median(samples) << " micro seconds\n";
    }
    {
        const int n{ 20 };
        std::vector<double> samples;
        std::cout << "std cold\n";
        for (int i = 0; i != n; ++i) {
            samples.push_back(std_cold());
        }
        std::cout << uc::get_median(samples) << " micro seconds\n";
    }
    {
        const int n{ 20 };
        std::vector<double> samples;
        std::cout << "std warm\n";
        for (int i = 0; i != n; ++i) {
            samples.push_back(std_warm());
        }
        std::cout << uc::get_median(samples) << " micro seconds\n";
    }

    {
        const int n{ 20 };
        std::vector<double> samples;
        std::cout << "uc seg simple\n";
        for (int i = 0; i != n; ++i) {
            samples.push_back(uc_seg_simple());
        }
        std::cout << uc::get_median(samples) << " micro seconds\n";
    }
    {
        const int n{ 20 };
        std::vector<double> samples;
        std::cout << "uc seg cold\n";
        for (int i = 0; i != n; ++i) {
            samples.push_back(uc_seg_cold());
        }
        std::cout << uc::get_median(samples) << " micro seconds\n";
    }
    {
        const int n{ 20 };
        std::vector<double> samples;
        std::cout << "uc seg warm\n";
        for (int i = 0; i != n; ++i) {
            samples.push_back(uc_seg_warm());
        }
        std::cout << uc::get_median(samples) << " micro seconds\n";
    }

    {
        const int n{ 20 };
        std::vector<double> samples;
        std::cout << "uc simple\n";
        for (int i = 0; i != n; ++i) {
            samples.push_back(uc_simple());
        }
        std::cout << uc::get_median(samples) << " micro seconds\n";
    }
    {
        const int n{ 20 };
        std::vector<double> samples;
        std::cout << "uc cold\n";
        for (int i = 0; i != n; ++i) {
            samples.push_back(uc_cold());
        }
        std::cout << uc::get_median(samples) << " micro seconds\n";
    }
    {
        const int n{ 20 };
        std::vector<double> samples;
        std::cout << "uc warm\n";
        for (int i = 0; i != n; ++i) {
            samples.push_back(uc_warm());
        }
        std::cout << uc::get_median(samples) << " micro seconds\n";
    }
    //{
    //    const int n{ 20 };
    //    std::vector<double> samples;
    //    std::cout << "raw loop\n";
    //    for (int i = 0; i != n; ++i) {
    //        samples.push_back(raw_loop());
    //    }
    //    std::cout << uc::get_median(samples) << " micro seconds\n";
    //}
}
