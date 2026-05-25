// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include <concepts>
#include <functional>
#include <iterator>
#include <type_traits>

export module uc.algorithm:find;
import uc.iterator;

namespace uc {
    struct find_if_fn {
        template<
            std::input_iterator I, std::sentinel_for<I> S,
            typename Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred
        >
        I operator()(I first, S last, Pred pred, Proj proj = {}) const {
            return uc::advance_while(
                std::move(first), std::move(last),
                [&](std::iter_reference_t<I> e) {
                    return !std::invoke(pred, std::invoke(proj, e));
                }
            ).pos;
        }
    };
    export
    constexpr find_if_fn find_if{};

    struct find_if_not_fn {
        template<
            std::input_iterator I, std::sentinel_for<I> S,
            typename Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred
        >
        I operator()(I first, S last, Pred pred, Proj proj = {}) const {
            return uc::advance_while(
                std::move(first), std::move(last),
                [&](std::iter_reference_t<I> e) {
                    return std::invoke(pred, std::invoke(proj, e));
                }
            ).pos;
        }
    };
    export
    constexpr find_if_not_fn find_if_not{};
}