// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include <functional>
#include <ranges>
#include <type_traits>

export module uc.adaptors:misc;

namespace uc {
    template<typename T>
    concept simple_view =
        std::ranges::view<T> &&
        std::ranges::range<const T> &&
        std::same_as<
            std::ranges::iterator_t<T>,
            std::ranges::iterator_t<const T>
        > &&
        std::same_as<
            std::ranges::sentinel_t<T>,
            std::ranges::sentinel_t<const T>
        >;

    template<bool B, typename T>
    using maybe_const_t = std::conditional_t<B, const T, T>;

    template <typename T>
    auto auto_ref(T&& val) {
        using non_ref_t = std::remove_reference_t<T>;
        if constexpr (std::is_const_v<non_ref_t>) {
            return std::cref(std::forward<T>(val));
        }
        else {
            return std::ref(std::forward<T>(val));
        }
    }
}