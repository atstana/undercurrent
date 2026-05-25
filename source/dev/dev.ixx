// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include <vector>
#include <random>
#include <cstddef>
#include <algorithm>

export module uc.dev;

namespace uc {
    export
    auto get_rng(std::size_t seed = std::random_device{}()) {
        std::mt19937_64 engine{ seed };
        return engine;
    }

    export
    template<typename Int = int>
    auto vrand(std::size_t n, std::size_t seed = std::random_device{}()) {
        std::vector<Int> result;
        result.reserve(n);
        auto rng = get_rng(seed);
        for (std::size_t i = 0; i != n; ++i) {
            result.push_back((Int)rng());
        }
        return result;
    }

    export
    template<typename Int = int>
    auto vrand(
        Int min,
        Int max,
        std::size_t seed = std::random_device{}()
    ) {
        if (max < min) {
            std::swap(min, max);
        }

        std::vector<Int> result;
        result.reserve(max - min);
        auto rng = get_rng(seed);
        std::uniform_int_distribution<Int> dist{ min, max };
        for (std::size_t i = 0; i != (max - min); ++i) {
            result.push_back(dist(rng));
        }
        return result;
    }

    export
    template<typename Int = int>
    auto vrand_unique(
        Int min,
        Int max,
        std::size_t seed = std::random_device{}()
    ) {
        if (max < min) {
            std::swap(min, max);
        }

        std::vector<Int> result;
        result.reserve(max - min);
        for (Int i = min; i != max; ++i) {
            result.push_back(i);
        }
        std::shuffle(
            result.begin(), result.end(),
            get_rng(seed)
        );
        return result;
    }

    export
    template<typename R>
    auto get_median(R& r) {
        if (r.empty()) return 0.0;
        std::size_t n = r.size();
        std::sort(r.begin(), r.end());
        if (n % 2 == 0) {
            return (r[n / 2 - 1] + r[n / 2]) / 2.0;
        } else {
            return r[n / 2];
        }
    }
}