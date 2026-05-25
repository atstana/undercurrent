// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include <concepts>
#include <functional>
#include <iterator>
#include <type_traits>

export module uc.iterator:basic_const_iterator;
import :iteration;
import :misc;
import uc.tag_invoke;

namespace uc {
    export
    template<std::input_iterator I>
    class basic_const_iterator;

    export
    template<typename T>
    using const_iterator = std::conditional_t<
        constant_iterator<T>, T, basic_const_iterator<T>
    >; 

    template<template<typename> typename T, typename I>
    constexpr bool is_a_basic_const_iterator(const T<I>& t) noexcept {
        return requires {
            requires std::is_same_v<T<I>, basic_const_iterator<I>>;
        };
    };

    template<typename T>
    concept not_a_const_iterator =
        requires(T&& t) {
            requires !uc::is_a_basic_const_iterator(std::forward<T>(t));
        };

    export
    template<std::input_iterator I>
    class basic_const_iterator {
    private:
        I current_;

    public:
        using value_type = std::iter_value_t<I>;
        using reference = uc::iter_const_reference_t<I>;
        using difference_type = std::iter_difference_t<I>;
        using iterator_concept = decltype(
            uc::calc_iter_concept<I, std::contiguous_iterator_tag>()
        );
        using iterator_category = decltype(
            uc::calc_iter_cat<I>()
        );

        constexpr basic_const_iterator()
        noexcept(std::is_nothrow_default_constructible_v<I>)
        requires std::is_default_constructible_v<I>
        : current_{} {}

        constexpr basic_const_iterator(I current)
        noexcept(std::is_nothrow_move_constructible_v<I>)
        : current_{ std::move(current) } {}

        template<std::convertible_to<I> T>
        constexpr basic_const_iterator(basic_const_iterator<T> other)
        noexcept(noexcept(I{ std::move(other).base() }))
        : current_{ std::move(other).base() } {}

        template<typename T>
        requires (
            !std::is_same_v<std::decay_t<T>, basic_const_iterator> &&
            std::convertible_to<T, I>
        )
        constexpr basic_const_iterator(T&& t)
        noexcept(std::is_nothrow_constructible_v<I, T>)
        : current_{ std::forward<T>(t) } {}

        template<not_a_const_iterator CI>
        requires
            constant_iterator<CI> &&
            std::convertible_to<const I&, CI>
        constexpr operator CI() const& {
            return current_;
        }
        template<not_a_const_iterator CI>
        requires
            constant_iterator<CI> &&
            std::convertible_to<const I&, CI>
        constexpr operator CI() && {
            return std::move(current_);
        }

        constexpr const I& base() const& noexcept {
            return current_;
        }
        constexpr I base() && {
            return std::move(current_);
        }

        constexpr reference operator*() const {
            return static_cast<reference>(*current_);
        }

        constexpr reference operator[](difference_type d) const
        requires std::random_access_iterator<I>
        {
            return static_cast<reference>(current_[d]);
        }

        constexpr basic_const_iterator& operator++() {
            ++current_;
            return *this;
        }
        constexpr basic_const_iterator operator++(int) {
            auto result{ *this };
            this->operator++();
            return result;
        }
        constexpr basic_const_iterator& operator+=(difference_type d)
        requires std::random_access_iterator<I>
        {
            current_ += d;
            return *this;
        }

        constexpr basic_const_iterator& operator--() {
            --current_;
            return *this;
        }
        constexpr basic_const_iterator operator--(int) {
            auto result{ *this };
            this->operator--();
            return result;
        }
        constexpr basic_const_iterator& operator-=(difference_type d)
        requires std::random_access_iterator<I>
        {
            current_ -= d;
            return *this;
        }

        friend constexpr basic_const_iterator
        operator+(const basic_const_iterator& lhs, difference_type rhs)
        requires std::random_access_iterator<I>
        {
            return lhs.base() + rhs;
        }

        friend constexpr basic_const_iterator
        operator+(difference_type lhs, const basic_const_iterator& rhs)
        requires std::random_access_iterator<I>
        {
            return rhs + lhs.base();
        }

        friend constexpr basic_const_iterator
        operator-(const basic_const_iterator& lhs, difference_type rhs)
        requires std::random_access_iterator<I>
        {
            return lhs.base() - rhs;
        }

        template<std::sized_sentinel_for<I> S>
        constexpr difference_type operator-(const S& s) const {
            return base() - s ;
        }
        template<not_a_const_iterator S>
        requires std::sized_sentinel_for<S, I>
        friend constexpr difference_type
        operator-(const S& s, const basic_const_iterator& rhs) {
            return s - rhs.base();
        }

        template<std::sentinel_for<I> S>
        constexpr bool operator==(const S& s) const
        {
            return base() == s;
        }

        constexpr bool operator<(const basic_const_iterator& rhs) const
        requires std::random_access_iterator<I>
        {
            return base() < rhs.base();
        }
        constexpr bool operator<=(const basic_const_iterator& rhs) const
        requires std::random_access_iterator<I>
        {
            return base() <= rhs.base();
        }

        constexpr bool operator>(const basic_const_iterator& rhs) const
        requires std::random_access_iterator<I>
        {
            return base() > rhs.base();
        }
        constexpr bool operator>=(const basic_const_iterator& rhs) const
        requires std::random_access_iterator<I>
        {
            return base() >= rhs.base();
        }

        constexpr auto operator<=>(const basic_const_iterator& rhs) const
        requires
            std::random_access_iterator<I> &&
            std::three_way_comparable<I>
        {
            return base() <=> rhs.base();
        }

        template<typename Rhs>
        requires (
            !std::is_same_v<Rhs, basic_const_iterator> &&
            std::totally_ordered_with<I, Rhs> &&
            std::random_access_iterator<I>
        )
        constexpr bool operator<(const Rhs& rhs) const {
            return base() < rhs;
        }
        template<typename Rhs>
        requires (
            !std::is_same_v<Rhs, basic_const_iterator> &&
            std::totally_ordered_with<I, Rhs> &&
            std::random_access_iterator<I>
        )
        constexpr bool operator<=(const Rhs& rhs) const {
            return base() <= rhs;
        }

        template<typename Rhs>
        requires (
            !std::is_same_v<Rhs, basic_const_iterator> &&
            std::totally_ordered_with<I, Rhs> &&
            std::random_access_iterator<I>
        )
        constexpr bool operator>(const Rhs& rhs) const {
            return base() > rhs;
        }
        template<typename Rhs>
        requires (
            !std::is_same_v<Rhs, basic_const_iterator> &&
            std::totally_ordered_with<I, Rhs> &&
            std::random_access_iterator<I>
        )
        constexpr bool operator>=(const Rhs& rhs) const {
            return base() >= rhs;
        }

        template<typename Rhs>
        requires (
            !std::is_same_v<Rhs, basic_const_iterator> &&
            std::totally_ordered_with<I, Rhs> &&
            std::three_way_comparable_with<I, Rhs>
        )
        constexpr auto operator<=>(const Rhs& rhs) const {
            return base() <=> rhs;
        }

        template<not_a_const_iterator Lhs> 
        requires
            std::random_access_iterator<I> &&
            std::totally_ordered_with<I, Lhs>
        friend constexpr bool
        operator<(const I& lhs, const basic_const_iterator& rhs) {
            return lhs < rhs.base();
        }
        template<not_a_const_iterator Lhs> 
        requires
            std::random_access_iterator<I> &&
            std::totally_ordered_with<I, Lhs>
        friend constexpr bool
        operator<=(const I& lhs, const basic_const_iterator& rhs) {
            return lhs <= rhs.base();
        }

        template<not_a_const_iterator Lhs> 
        requires
            std::random_access_iterator<I> &&
            std::totally_ordered_with<I, Lhs>
        friend constexpr bool
        operator>(const I& lhs, const basic_const_iterator& rhs) {
            return lhs > rhs.base();
        }
        template<not_a_const_iterator Lhs> 
        requires
            std::random_access_iterator<I> &&
            std::totally_ordered_with<I, Lhs>
        friend constexpr bool
        operator>=(const I& lhs, const basic_const_iterator& rhs) {
            return lhs >= rhs.base();
        }

        friend constexpr reference
        tag_invoke(tag_t<uc::read_and_inc>, basic_const_iterator& cur) {
            return uc::read_and_inc(cur.current_);
        }
        friend constexpr reference
        tag_invoke(tag_t<uc::dec_and_read>, basic_const_iterator& cur) {
            return uc::dec_and_read(cur.current_);
        }

        template<
            indirect_non_regular_predicate<I> P,
            direction_tag Dir
        >
        requires advancable<I, I, Dir>
        friend constexpr advance_while_result<basic_const_iterator>
        tag_invoke(
            tag_t<uc::advance_while>,
            basic_const_iterator first,
            basic_const_iterator last,
            P pred,
            Dir dir
        ) {
            auto [pos, flag] = uc::advance_while(
                std::move(first).base(),
                std::move(last).base(),
                [pred = std::move(pred)](std::iter_reference_t<I> e) {
                    return std::invoke(pred, static_cast<reference>(e));
                },
                dir
            );
            return {
                basic_const_iterator{ std::move(pos) },
                flag
            };
        }
    };
}