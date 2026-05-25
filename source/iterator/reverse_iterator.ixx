// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include <concepts>
#include <iterator>
#include <type_traits>

export module uc.iterator:reverse_iterator;
import :iteration;
import :misc;
import uc.tag_invoke;

namespace uc {
    export
    template<std::bidirectional_iterator I>
    class reverse_iterator {
    private:
        I current_;

    public:
        using iterator_type = I;
        using value_type = std::iter_value_t<I>;
        using reference = std::iter_reference_t<I>;
        using difference_type = std::iter_difference_t<I>;
        using iterator_concept = decltype(
            uc::calc_iter_concept<I, std::random_access_iterator_tag>()
        );
        using iterator_category = decltype(
            uc::calc_iter_cat<I>()
        );

        constexpr reverse_iterator()
        noexcept(std::is_nothrow_default_constructible_v<I>)
        requires std::is_default_constructible_v<I>
        : current_{} {}

        explicit constexpr reverse_iterator(I current)
        noexcept(std::is_nothrow_move_constructible_v<I>)
        : current_{ std::move(current) } {}

        template<typename U>
        requires(
            !std::is_same_v<std::remove_cvref_t<U>, reverse_iterator> &&
            std::convertible_to<U, I>
        )
        constexpr reverse_iterator(const reverse_iterator<U>& other)
        noexcept(std::is_nothrow_constructible_v<I, U>)
        : current_{ other.base() } {}

        constexpr const I& base() const& noexcept {
            return current_;
        }
        constexpr I base() && {
            return std::move(current_);
        }

        constexpr reference operator*() const {
            auto temp{ current_ };
            return *(--temp);
        }

        constexpr reference operator[](difference_type d) const
        requires std::random_access_iterator<I>
        {
            return current_[-d - 1];
        }

        constexpr reverse_iterator& operator++() {
            --current_;
            return *this;
        }
        constexpr reverse_iterator operator++(int) {
            auto result{ *this };
            this->operator++();
            return result;
        }
        constexpr reverse_iterator& operator+=(difference_type d)
        requires std::random_access_iterator<I>
        {
            current_ -= d;
            return *this;
        }
        constexpr reverse_iterator operator+(difference_type d) const
        requires std::random_access_iterator<I>
        {
            return reverse_iterator{ current_ - d } ;
        }

        constexpr reverse_iterator& operator--() {
            ++current_;
            return *this;
        }
        constexpr reverse_iterator operator--(int) {
            auto result{ *this };
            this->operator--();
            return result;
        }
        constexpr reverse_iterator& operator-=(difference_type d)
        requires std::random_access_iterator<I>
        {
            current_ += d;
            return *this;
        }
        constexpr reverse_iterator operator-(difference_type d) const
        requires std::random_access_iterator<I>
        {
            return reverse_iterator{ current_ + d } ;
        }

        friend constexpr reference
        tag_invoke(tag_t<uc::read_and_inc>, reverse_iterator& cur) {
            return uc::dec_and_read(cur.current_);
        }
        friend constexpr reference
        tag_invoke(tag_t<uc::dec_and_read>, reverse_iterator& cur) {
            return uc::read_and_inc(cur.current_);
        }

        template<
            indirect_non_regular_predicate<I> P,
            direction_tag Dir
        >
        friend constexpr advance_while_result<reverse_iterator>
        tag_invoke(
            tag_t<uc::advance_while>,
            reverse_iterator first,
            reverse_iterator last,
            P pred,
            Dir dir
        ) {
            auto [pos, flag] = uc::advance_while(
                std::move(last).base(),
                std::move(first).base(),
                std::move(pred),
                !dir
            );
            return {
                reverse_iterator{
                    flag ? std::move(pos) : std::move(++pos)
                },
                flag
            };
        }
    };

    export
    template<typename I>
    constexpr reverse_iterator<I>
    operator+(
        typename reverse_iterator<I>::difference_type lhs,
        const reverse_iterator<I>& rhs
    )
    requires std::random_access_iterator<I>
    {
        return rhs + lhs;
    }

    export
    template<typename LI, typename RI>
    constexpr auto
    operator-(const reverse_iterator<LI>& lhs, const reverse_iterator<RI>& rhs)
    requires std::sized_sentinel_for<RI, LI>
    {
        return rhs.base() - lhs.base();
    }

    export
    template<typename LI, typename RI>
    constexpr bool
    operator==(const reverse_iterator<LI>& lhs, const reverse_iterator<RI>& rhs) {
        return lhs.base() == rhs.base();
    }

    export
    template<typename LI, typename RI>
    constexpr bool
    operator<(const reverse_iterator<LI>& lhs, const reverse_iterator<RI>& rhs)
    requires std::totally_ordered_with<LI, RI>
    {
        return lhs.base() > rhs.base();
    }
    export
    template<typename LI, typename RI>
    constexpr bool
    operator<=(const reverse_iterator<LI>& lhs, const reverse_iterator<RI>& rhs)
    requires std::totally_ordered_with<LI, RI>
    {
        return lhs.base() >= rhs.base();
    }

    export
    template<typename LI, typename RI>
    constexpr bool
    operator>(const reverse_iterator<LI>& lhs, const reverse_iterator<RI>& rhs)
    requires std::totally_ordered_with<LI, RI>
    {
        return lhs.base() < rhs.base();
    }
    export
    template<typename LI, typename RI>
    constexpr bool
    operator>=(const reverse_iterator<LI>& lhs, const reverse_iterator<RI>& rhs)
    requires std::totally_ordered_with<LI, RI>
    {
        return lhs.base() <= rhs.base();
    }

    export
    template<typename LI, typename RI>
    requires std::three_way_comparable_with<RI, LI>
    constexpr auto
    operator<=>(const reverse_iterator<LI>& lhs, const reverse_iterator<RI>& rhs) {
        return rhs.base() <=> lhs.base();
    }


    export
    template<std::bidirectional_iterator I>
    constexpr auto
    make_reverse_iterator(I&& i) {
        return reverse_iterator<std::decay_t<I>>{
            std::forward<I>(i)
        };
    }

    template<typename T>
    constexpr bool is_a_reverse_iterator_v{ false };

    template<typename I>
    constexpr bool is_a_reverse_iterator_v<reverse_iterator<I>>{ true };

    export
    template<typename I>
    requires
        is_a_reverse_iterator_v<std::decay_t<I>> ||
        std::bidirectional_iterator<std::decay_t<I>>
    constexpr auto
    make_iterator_reversed(I&& i) {
        if constexpr (is_a_reverse_iterator_v<std::decay_t<I>>) {
            return std::forward<I>(i).base();
        }
        else {
            return reverse_iterator{ std::forward<I>(i) };
        }
    }
}