// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include <concepts>
#include <utility>

export module uc.concepts;

namespace uc {
    template<typename T>
    concept boolean_testable =
        std::convertible_to<T, bool> &&
        requires(T&& t) {
            { !std::forward<T>(t) } -> std::convertible_to<bool>;
        };

    export
    template<typename P, typename Arg>
    concept non_regular_predicate =
        std::copy_constructible<P> &&
        std::invocable<P&, Arg> &&
        boolean_testable<
            std::invoke_result_t<
                P&, Arg
            >
        >;
}