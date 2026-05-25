// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include <algorithm>
#include <concepts>
#include <functional>
#include <iterator>
#include <type_traits>

export module uc.algorithm:for_each;
import uc.iterator;

namespace uc {
    export
    template<typename I, typename F>
    using for_each_result = std::ranges::in_fun_result<I, F>;

    export
    template<
        std::input_iterator I,
        std::sentinel_for<I> S,
        typename Proj = std::identity,
        std::indirectly_unary_invocable<std::projected<I, Proj>> F
    >
    constexpr for_each_result<I, F>
    for_each( I first, S last, F f, Proj proj = {}) {
        return {
            uc::advance_while(
                std::move(first), std::move(last),
                [&] (std::iter_reference_t<I> e) {
                    std::invoke(f, std::invoke(proj, e));
                    return true;
                }
            ).pos,
            std::move(f)
        };
    }
}