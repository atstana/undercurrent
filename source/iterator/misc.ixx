// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include <concepts>
#include <iterator>
#include <type_traits>

export module uc.iterator:misc;

namespace uc {
    export
    template<typename T>
    using iter_const_reference_t =
        std::common_reference_t<
            const std::iter_value_t<T>&&,
            std::iter_reference_t<T>
        >;

    template<typename I>
    concept nothrow_forward_iterator = 
        std::forward_iterator<I> &&
        requires {
            requires
                std::is_lvalue_reference_v<
                    std::iter_reference_t<I>
                > &&
                std::same_as<
                    std::remove_cvref_t<std::iter_reference_t<I>>,
                    std::iter_value_t<I>
                >;
        }; 

    template<typename T>
    concept constant_iterator =
        std::input_iterator<T> &&
        std::same_as<
            iter_const_reference_t<T>,
            std::iter_reference_t<T>
        >;

    template<typename I, typename Tag>
    constexpr auto calc_iter_concept() noexcept {
        if constexpr (
            std::contiguous_iterator<I> &&
            std::is_base_of_v<std::contiguous_iterator_tag, Tag>
        ) {
            return std::contiguous_iterator_tag{};
        }
        else if constexpr (
            std::random_access_iterator<I> &&
            std::is_base_of_v<Tag, std::random_access_iterator_tag>
        ) {
            return std::random_access_iterator_tag{};
        }
        else if constexpr (
            std::bidirectional_iterator<I> &&
            std::is_base_of_v<Tag, std::bidirectional_iterator_tag>
        ) {
            return std::bidirectional_iterator_tag{};
        }
        else if constexpr (
            std::forward_iterator<I> &&
            std::is_base_of_v<Tag, std::forward_iterator_tag>
        ) {
            return std::forward_iterator_tag{};
        }
        else {
            return std::input_iterator_tag{};
        }
    }
    template<typename I>
    constexpr auto calc_iter_cat() noexcept {
        if constexpr (!nothrow_forward_iterator<I>) {
            return std::input_iterator_tag{};
        }
        else if constexpr (
            std::is_same_v<
                typename std::iterator_traits<I>::iterator_category,
                std::contiguous_iterator_tag
            >
        ){
            return std::random_access_iterator_tag{};
        }
        else {
            using result_t =
                typename std::iterator_traits<I>::iterator_category;
            return result_t{};
        }
    }
}