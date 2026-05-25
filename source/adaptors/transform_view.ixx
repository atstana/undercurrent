// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include <concepts>
#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

export module uc.adaptors:transform_view;
import :misc;
import :movable_box;
import uc.iterator;
import uc.tag_invoke;

namespace uc {
    namespace sr = std::ranges;
    namespace sv = std::ranges::views;

    export
    template<sr::view V, typename F>
    class transform_view {
    private:
        V base_;
        movable_box<F> f_;

    public:
        constexpr transform_view()
        requires
            std::is_default_constructible_v<V> &&
            std::is_default_constructible_v<F>
        : base_{}, f_{} {}

        constexpr transform_view(V v, F f) :
            base_{ std::move(v) },
            f_{ std::move(f) } {}

        constexpr V base() const&
        requires std::copy_constructible<V>
        {
            return base_;
        }
        constexpr V base() && {
            return std::move(base_);
        }

        template<bool B>
        class iterator;

        template<bool B>
        class sentinel;

        constexpr iterator<false> begin()
        {
            return iterator<false>{ this, sr::begin(base_) };
        }
        constexpr iterator<true> begin() const
        requires
            sr::range<const V> &&
            std::regular_invocable<
                const F&,
                sr::range_reference_t<const V>
            >
        {
            return iterator<true>{ this, sr::begin(base_) };
        }

        constexpr iterator<false> end()
        requires sr::common_range<V>
        {
            return iterator<false>{ this, sr::end(base_) };
        }
        constexpr iterator<true> end() const
        requires
            sr::common_range<const V> &&
            std::regular_invocable<
                const F&,
                sr::range_reference_t<const V>
            >
        {
            return iterator<true>{ this, sr::end(base_) };
        }

        constexpr sentinel<false> end() {
            return sentinel<false>{ sr::end(base_) };
        }
        constexpr sentinel<true> end() const
        requires
            sr::range<const V> &&
            std::regular_invocable<
                const F&,
                sr::range_reference_t<const V>
            >
        {
            return sentinel<true>{ sr::end(base_) };
        }
    };

    template<sr::view V, typename F>
    template<bool B>
    class transform_view<V, F>::iterator {
    private:
        friend iterator<!B>;
        using parent_type = maybe_const_t<B, transform_view<V, F>>;
        using base_view = maybe_const_t<B, V>;
        using base_iterator = sr::iterator_t<base_view>;

    public:
        using reference = std::invoke_result_t<
            F, std::iter_reference_t<base_iterator>
        >;
        using value_type = std::remove_cvref_t<reference>;
        using difference_type = std::iter_difference_t<base_iterator>;

    private:
        parent_type* parent_;
        base_iterator current_;

        constexpr decltype(auto)
        fun() noexcept {
            return *(parent_->f_);
        } 

    public:
        constexpr iterator() : parent_{ nullptr }, current_{} {}

        constexpr iterator(
            parent_type* parent,
            base_iterator current
        )
        : parent_{ parent }, current_{ std::move(current) } {}

        constexpr iterator(iterator<!B> other)
        requires (
            B &&
            std::convertible_to<
                sr::iterator_t<V>,
                sr::iterator_t<const V>
            >
        ) :
            parent_{ other.parent_ },
            current_{ std::move(other.current_) } {}

        constexpr const base_iterator& base() const& noexcept {
            return current_;
        }
        constexpr base_iterator base() && {
            return std::move(current_);
        }

        constexpr reference operator*() const {
            return std::invoke(*(parent_->f_), *current_);
        }

        constexpr iterator& operator++() {
            ++current_;
            return *this;
        }
        constexpr iterator operator++(int)
        requires std::forward_iterator<base_iterator>
        {
            const auto result{ *this };
            this->operator++();
            return result;
        }
        constexpr void operator++(int)
        requires (!std::forward_iterator<base_iterator>)
        {
            ++(*this);
        }
        constexpr iterator& operator+=(difference_type d)
        requires std::random_access_iterator<base_iterator>
        {
            current_ += d;
            return *this;
        }

        constexpr iterator& operator--()
        requires std::bidirectional_iterator<base_iterator>
        {
            --current_;
            return *this;
        }
        constexpr iterator operator--(int)
        requires std::bidirectional_iterator<base_iterator>
        {
            const auto result{ *this };
            this->operator--();
            return result;
        }
        constexpr iterator& operator-=(difference_type d)
        requires std::random_access_iterator<base_iterator>
        {
            current_ -= d;
            return *this;
        }

        friend constexpr iterator
        operator+(const iterator& lhs, difference_type rhs)
        requires std::random_access_iterator<base_iterator>
        {
            return iterator{ lhs.base() + rhs };
        }
        friend constexpr iterator
        operator+(difference_type lhs, const iterator& rhs)
        requires std::random_access_iterator<base_iterator>
        {
            return iterator{ rhs + lhs };
        }

        friend constexpr iterator
        operator-(const iterator& lhs, difference_type rhs)
        requires std::random_access_iterator<base_iterator>
        {
            return iterator{ lhs.base() - rhs };
        }
        friend constexpr difference_type
        operator-(const iterator& lhs, const iterator& rhs)
        requires std::random_access_iterator<base_iterator>
        {
            return lhs.base() - rhs.base();
        }

        friend constexpr bool
        operator==(const iterator& lhs, const iterator& rhs) {
            return lhs.base() == rhs.base();
        }

        friend constexpr bool
        operator<(const iterator& lhs, const iterator& rhs)
        requires std::random_access_iterator<base_iterator>
        {
            return lhs.base() < rhs.base();
        }
        friend constexpr bool
        operator<=(const iterator& lhs, const iterator& rhs)
        requires std::random_access_iterator<base_iterator>
        {
            return lhs.base() <= rhs.base();
        }

        friend constexpr bool
        operator>(const iterator& lhs, const iterator& rhs)
        requires std::random_access_iterator<base_iterator>
        {
            return lhs.base() > rhs.base();
        }
        friend constexpr bool
        operator>=(const iterator& lhs, const iterator& rhs)
        requires std::random_access_iterator<base_iterator>
        {
            return lhs.base() >= rhs.base();
        }

        friend constexpr auto
        operator<=>(const iterator& lhs, const iterator& rhs)
        requires
            std::random_access_iterator<base_iterator> &&
            std::three_way_comparable<base_iterator>

        {
            return lhs.base() <=> rhs.base();
        }

        friend constexpr reference
        tag_invoke(tag_t<uc::read_and_inc>, iterator& cur) {
            return std::invoke(cur.fun(), uc::read_and_inc(cur.current_));
        }
        friend constexpr reference
        tag_invoke(tag_t<uc::dec_and_read>, iterator& cur) {
            return std::invoke(cur.fun(), uc::dec_and_read(cur.current_));
        }

        template<
            std::sentinel_for<iterator> S,
            indirect_non_regular_predicate<base_iterator> P,
            direction_tag Dir
        >
        requires advancable<
            base_iterator,
            std::remove_reference_t<decltype(std::declval<S>().base())>,
            Dir
        >
        friend constexpr advance_while_result<iterator>
        tag_invoke(
            tag_t<uc::advance_while>,
            iterator first,
            S last,
            P pred,
            Dir dir
        ) {
            if constexpr (std::is_same_v<P, skip_t>) {
                auto [pos, flag] = uc::advance_while(
                    std::move(first).base(),
                    std::move(last).base(),
                    std::move(pred),
                    dir
                );
                return {
                    iterator{ first.parent_, std::move(pos) },
                    flag
                };
            }
            else {
                auto [pos, flag] = uc::advance_while(
                    std::move(first).base(),
                    std::move(last).base(),
                    [&, pred = std::move(pred)]
                    (std::iter_reference_t<base_iterator> e) {
                        return std::invoke(
                            pred,
                            std::invoke(first.fun(), e)
                        );
                    },
                    dir
                );
                return {
                    iterator{ first.parent_, std::move(pos) },
                    flag
                };
            }
        }
    };

    template<sr::view V, typename F>
    template<bool B>
    class transform_view<V, F>::sentinel {
    private:
        friend sentinel<!B>;
        using base_view = std::conditional_t<
            B,
            const V,
            V
        >;
        using base_iterator = sr::iterator_t<base_view>;
        using base_sentinel = sr::sentinel_t<base_view>;
        using difference_type = std::iter_difference_t<base_iterator>;

        base_sentinel sen_;

    public:
        constexpr sentinel() : sen_{} {}

        explicit constexpr sentinel(base_sentinel sen) :
            sen_{ std::move(sen) } {}

        constexpr const base_sentinel& base() const& noexcept {
            return sen_;
        }
        constexpr base_sentinel base() && {
            return std::move(sen_);
        }

        constexpr friend bool
        operator==(const iterator<B>& lhs, const sentinel& rhs) {
            return lhs.base() == rhs.base();
        }

        constexpr friend difference_type
        operator-(const sentinel& lhs, const iterator<B>& rhs)
        requires
            requires { { lhs.sen_ - rhs.base() } -> std::same_as<difference_type>; }
        {
            return lhs.sen_ - rhs.base();
        }
    };

    template<typename R, typename F>
    transform_view(R&&, F&&) -> transform_view<
        sv::all_t<R>, std::decay_t<F>
    >;

    struct transform_fn {
        template<typename F>
        class closure {
        private:
            F f_;

        public:
            template<typename FF>
            constexpr closure(FF&& f) : f_{ std::forward<FF>(f) } {}

            template<sr::viewable_range R>
            friend constexpr auto operator|(R&& r, closure&& clo) {
                return transform_view{
                    std::forward<R>(r),
                    std::forward<F>(clo.f_)
                };
            }
        };

        template<typename F>
        constexpr closure<F> operator()(F&& f) const {
            return closure<F>{ std::forward<F>(f) };
        }
    };
    export
    constexpr transform_fn transform{};
}

namespace std::ranges {
    template<typename R, typename F>
    constexpr bool enable_view<uc::transform_view<R, F>>{ true };
}