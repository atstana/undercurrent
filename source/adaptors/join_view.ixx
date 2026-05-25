// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include <cassert>
#include <concepts>
#include <functional>
#include <iterator>
#include <optional>
#include <ranges>
#include <type_traits>
#include <utility>

export module uc.adaptors:join_view;
import :misc;
import :movable_box;
import :non_propagating_cache;
import uc.iterator;
import uc.tag_invoke;

namespace uc {
    namespace sr = std::ranges;
    namespace sv = std::ranges::views;
    
    export
    template<sr::input_range V>
    requires
        sr::view<V> &&
        sr::input_range<sr::range_reference_t<V>>
    class join_view {
    private:
        using inner_range = sr::range_reference_t<V>;

        struct empty{};
        static constexpr auto select_inner_range_storage() {
            if constexpr (std::is_reference_v<inner_range>) {
                return std::type_identity<empty>{};
            }
            else {
                return std::type_identity<non_propagating_cache<inner_range>>{};
            }
        };
        using inner_range_storage =
            typename decltype(select_inner_range_storage())::type;

        V base_;
        inner_range_storage inner_;

    public:
        constexpr join_view(V base) :
            base_{ std::move(base) },
            inner_{} {}
            
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

        constexpr auto begin() {
            if constexpr (
                simple_view<V> &&
                std::is_reference_v<sr::range_reference_t<V>>
            ) {
                return iterator<true>{ this, sr::begin(base_) };
            }
            else {
                return iterator<false>{ this, sr::begin(base_) };
            }
        }
        constexpr auto begin() const
        requires
            sr::input_range<const V> &&
            std::is_reference_v<sr::range_reference_t<const V>>
        {
            return iterator<true>{ this, sr::begin(base_) };
        }

        constexpr auto end() {
            if constexpr (
                sr::forward_range<V> &&
                std::is_reference_v<sr::range_reference_t<V>> &&
                sr::common_range<V> &&
                sr::common_range<sr::range_reference_t<V>>
            ) {
                return iterator<simple_view<V>>{ this, sr::end(base_) };
            }
            else {
                return sentinel<simple_view<V>>{ sr::end(base_) };
            }
        }
        constexpr auto end() const
        requires
            sr::input_range<const V> &&
            std::is_reference_v<sr::range_reference_t<const V>>
        {
            if constexpr (
                sr::forward_range<const V> &&
                std::is_reference_v<sr::range_reference_t<const V>> &&
                sr::common_range<const V> &&
                sr::common_range<sr::range_reference_t<const V>>
            ) {
                return iterator<true>{ this, sr::end(base_) };

            }
            else {
                return sentinel<true>{ sr::end(base_) };
            }
        }
    };

    template<sr::input_range V>
    requires
        sr::view<V> &&
        sr::input_range<sr::range_reference_t<V>>
    template<bool B>
    class join_view<V>::iterator {
    private:
        friend iterator<!B>;
        friend sentinel<B>;
        using parent_type = maybe_const_t<B, join_view<V>>;
        using outer_view = maybe_const_t<B, V>;
        using outer_iterator = sr::iterator_t<outer_view>;
        using inner_range = sr::range_reference_t<outer_view>;
        using inner_iterator = sr::iterator_t<
            std::remove_reference_t<inner_range>
        >;

        parent_type* parent_; 
        outer_iterator outer_;
        inner_iterator inner_;

        constexpr void satisfy() {
            if constexpr (std::is_reference_v<inner_range>) {
                while(outer_ != sr::end(parent_->base_)) {
                    inner_range temp{ *outer_ };
                    inner_ = sr::begin(temp);
                    if (inner_ == sr::end(temp)) {
                        ++outer_;
                    }
                    else {
                        return;
                    }
                }
                inner_ = inner_iterator{};
            }
            else {
                while(outer_ != sr::end(parent_->base_)) {
                    parent_->inner_.emplace(*outer_);
                    inner_ = sr::begin(*(parent_->inner_));
                    if (inner_ == sr::end(*(parent_->inner_))) {
                        ++outer_;
                    }
                    else {
                        return;
                    }
                }
            }
        }

        constexpr bool is_past_the_end() const {
            return outer_ == sr::end(parent_->base_);
        }

    public:
        using reference = std::iter_reference_t<inner_iterator>;
        using value_type = std::iter_value_t<inner_iterator>;
        using difference_type = std::common_type_t<
            std::iter_difference_t<outer_iterator>,
            std::iter_difference_t<inner_iterator>
        >;

        constexpr iterator()
        requires
            std::is_default_constructible_v<outer_iterator> &&
            std::is_default_constructible_v<inner_iterator>
        : parent_{ nullptr }, outer_{}, inner_{} {}

        constexpr iterator(
            parent_type* parent,
            outer_iterator outer
        ) :
            parent_{ parent },
            outer_{ std::move(outer) }
        {
            satisfy();
        }

        constexpr iterator(iterator<!B> other)
        requires (
            B &&
            std::convertible_to<
                sr::iterator_t<V>,
                outer_iterator
            > &&
            std::convertible_to<
                sr::iterator_t<inner_range>,
                inner_iterator
            >
        ) :
            parent_{ other.parent_ },
            outer_{ std::move(other.outer_) },
            inner_{ std::move(other.inner_) } {}

        constexpr reference
        operator*() const {
            return *inner_;
        }

        constexpr iterator&
        operator++() {
            if constexpr (std::is_reference_v<inner_range>) {
                ++inner_;
                if (inner_ == sr::end(*outer_)) {
                    ++outer_;
                    satisfy();
                }
            }
            else {
                ++inner_;
                if (inner_ == sr::end(*(parent_->inner_))) {
                    ++outer_;
                    satisfy();
                }
            }
            return *this;
        }
        constexpr void
        operator++(int) {
            this->operator++();
        }
        constexpr iterator
        operator++(int)
        requires
            std::is_reference_v<inner_range> &&
            std::forward_iterator<outer_iterator> &&
            std::forward_iterator<inner_iterator>
        {
            auto result{ *this };
            this->operator++();
            return result;
        }

        constexpr iterator&
        operator--()
        requires
            std::is_reference_v<inner_range> &&
            std::bidirectional_iterator<outer_iterator> &&
            std::bidirectional_iterator<inner_iterator>
        {
            if (outer_ == sr::end(parent_->base_)) {
                --outer_;
                inner_ = sr::end(*outer_);
            }
            while(inner_ == sr::begin(*outer_)) {
                --outer_;
                inner_ = sr::end(*outer_);
            }
            --inner_;
            return *this;
        }
        constexpr iterator
        operator--(int)
        requires
            std::is_reference_v<inner_range> &&
            std::bidirectional_iterator<outer_iterator> &&
            std::bidirectional_iterator<inner_iterator>
        {
            auto result{ *this };
            this->operator--();
            return result;
        }

        friend constexpr bool
        operator==(const iterator& lhs, const iterator& rhs)
        requires
            std::is_reference_v<inner_range> &&
            std::equality_comparable<outer_iterator> &&
            std::equality_comparable<inner_iterator>
        {
            return (
                (lhs.outer_ == rhs.outer_) &&
                (lhs.inner_ == rhs.inner_)
            );
        }

        template<
            indirect_non_regular_predicate<inner_iterator> P,
            direction_tag Dir
        >
        requires (
            std::is_reference_v<inner_range> &&
            advancable<outer_iterator, outer_iterator, Dir> &&
            advancable<inner_iterator, sr::sentinel_t<inner_range>, Dir>
        )
        friend constexpr advance_while_result<iterator>
        tag_invoke(
            tag_t<uc::advance_while>,
            iterator first,
            iterator last,
            P pred,
            Dir dir
        ) {
            if constexpr (std::is_same_v<P, skip_t>) {
                if constexpr (dir) {
                    return { std::move(last), true };
                }
                else {
                    return { std::move(first), true };
                }
            }
            else {
                if constexpr (dir) {
                    auto outer_pred = [&](inner_range ir) {
                        auto [pos_inner, flag_inner] = uc::advance_while(
                            sr::begin(ir), sr::end(ir),
                            pred, dir
                        );
                        first.inner_ = std::move(pos_inner);
                        return flag_inner;
                    };

                    if (first.outer_ != last.outer_) {
                        auto [pos_inner, flag_inner] = uc::advance_while(
                            first.inner_, sr::end(*first.outer_),
                            std::ref(pred), dir
                        );
                        if (!flag_inner) {
                            first.inner_ = std::move(pos_inner);
                            return { std::move(first), false };
                        }

                        ++first.outer_;
                        auto [pos_outer, flag_outer] = uc::advance_while(
                            first.outer_, last.outer_,
                            outer_pred, dir
                        );
                        first.outer_ = std::move(pos_outer);
                        if (!flag_outer) {
                            return { std::move(first), false };
                        }

                        if (first.is_past_the_end()) {
                            first.inner_ = inner_iterator{};
                        }
                        else {
                            first.inner_ = sr::begin(*first.outer_);
                        }
                    }
                    auto [pos_inner, flag_inner] = uc::advance_while(
                        first.inner_, last.inner_,
                        std::move(pred), dir
                    );
                    first.inner_ = std::move(pos_inner);
                    return { std::move(first), flag_inner };
                }
                else {
                    auto outer_pred = [&](inner_range ir) {
                        auto [pos_inner, flag_inner] = uc::advance_while(
                            sr::begin(ir), sr::end(ir),
                            pred, dir
                        );
                        last.inner_ = std::move(pos_inner);
                        return flag_inner;
                    };

                    if (first.outer_ != last.outer_) {
                        if (!last.is_past_the_end()) {
                            auto [pos_inner, flag_inner] = uc::advance_while(
                                sr::begin(*last.outer_), last.inner_,
                                std::ref(pred), dir
                            );
                            if (!flag_inner) {
                                last.inner_ = std::move(pos_inner);
                                return { std::move(last), false };
                            }
                        }

                        auto [pos_outer, flag_outer] = uc::advance_while(
                            sr::next(first.outer_), last.outer_,
                            outer_pred, dir
                        );
                        last.outer_ = std::move(pos_outer);
                        if (!flag_outer) {
                            return { std::move(last), false };
                        }

                        last.outer_ = first.outer_;
                        last.inner_ = sr::end(*first.outer_);
                    }
                    auto [pos_inner, flag_inner] = uc::advance_while(
                        first.inner_, last.inner_,
                        std::move(pred), dir
                    );
                    last.inner_ = std::move(pos_inner);
                    return { std::move(last), flag_inner };
                }
            }
        }
    };

    template<sr::input_range V>
    requires
        sr::view<V> &&
        sr::input_range<sr::range_reference_t<V>>
    template<bool B>
    class join_view<V>::sentinel {
    private:
        friend sentinel<!B>;
        using parent_type = maybe_const_t<B, join_view<V>>;
        using outer_view = maybe_const_t<B, V>;
        using outer_iterator = sr::iterator_t<outer_view>;
        using outer_sentinel = sr::sentinel_t<outer_view>;
        using inner_range = sr::range_reference_t<outer_view>;
        using inner_iterator = sr::iterator_t<
            std::remove_reference_t<inner_range>
        >;
        using reference = sr::range_reference_t<inner_range>;

        outer_sentinel sen_;

        static constexpr decltype(auto)
        outer(iterator<B>& it) noexcept {
            return (it.outer_);
        }
        static constexpr decltype(auto)
        inner(iterator<B>& it) noexcept {
            return (it.inner_);
        }
        static constexpr parent_type*
        parent(iterator<B>& it) noexcept {
            return it.parent_;
        }
        static constexpr decltype(auto)
        inner_storage(iterator<B>& it) noexcept {
            return (it.parent_->inner_);
        }

    public:
        constexpr sentinel()
        requires std::is_default_constructible_v<outer_sentinel>
        : sen_{} {}

        constexpr sentinel(outer_sentinel sen)
        : sen_{ std::move(sen) } {}

        constexpr sentinel(sentinel<!B> other)
        requires (
            B &&
            std::convertible_to<
                sr::sentinel_t<V>,
                outer_sentinel
            >
        )
        : sen_{ std::move(other.sen_) } {}

        friend constexpr bool
        operator==(const iterator<B>& lhs, const sentinel& rhs) {
            return outer(lhs) == rhs.sen_;
        }

        template<indirect_non_regular_predicate<inner_iterator> P>
        friend constexpr advance_while_result<iterator<B>>
        tag_invoke(
            tag_t<uc::advance_while>,
            iterator<B> first,
            sentinel last,
            P pred,
            forward_tag tag
        ) {
            if constexpr (std::is_same_v<P, skip_t>) {
                outer(first) = uc::next(outer(first), last.sen_);
                if constexpr (std::is_reference_v<inner_range>) {
                    inner(first) = inner_iterator{};
                }
                return { std::move(first), true };
            }
            else {
                auto outer_pred = [&](inner_range ir) {
                    auto&& ir_cached = [&]()
                        -> decltype(auto)
                    {
                        if constexpr (!std::is_reference_v<inner_range>) {
                            return inner_storage(first).emplace(std::move(ir));
                        }
                        else {
                            return ir;
                        }
                    }();

                    auto [pos_inner, flag_inner] = uc::advance_while(
                        sr::begin(ir_cached), sr::end(ir_cached),
                        pred
                    );
                    inner(first) = std::move(pos_inner);
                    return flag_inner;
                };

                if (outer(first) != last.sen_) {
                    auto&& ir = [&]()
                        -> decltype(auto)
                    {
                        if constexpr (!std::is_reference_v<inner_range>) {
                            return *inner_storage(first);
                        }
                        else {
                            return *outer(first);
                        }
                    }();
                    auto [pos_inner, flag_inner] = uc::advance_while(
                        inner(first), sr::end(ir),
                        std::ref(pred)
                    );
                    if (!flag_inner) {
                        inner(first) = std::move(pos_inner);
                        return { std::move(first), false };
                    }

                    ++outer(first);
                    auto [pos_outer, flag_outer] = uc::advance_while(
                        outer(first), last.sen_,
                        outer_pred 
                    );
                    outer(first) = std::move(pos_outer);
                    if (!flag_outer) {
                        return { std::move(first), false };
                    }

                    if constexpr (std::is_reference_v<inner_range>) {
                        inner(first) = inner_iterator{};
                    }
                }
                return { std::move(first), true };
            }
        }
    };

    template<typename R>
    join_view(R&&) -> join_view<sv::all_t<R>>;

    struct join_fn {
        template<sr::viewable_range R>
        friend constexpr auto operator|(R&& r, join_fn fact) {
            return join_view{ std::forward<R>(r) };
        }
    };

    export
    constexpr join_fn join{};
}

namespace std::ranges {
    template<typename R>
    constexpr bool enable_view<uc::join_view<R>>{ true };
}