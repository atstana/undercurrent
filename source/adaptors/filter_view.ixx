// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include <functional>
#include <iterator>
#include <ranges>
#include <utility>

export module uc.adaptors:filter_view;
import :misc;
import :movable_box;
import :non_propagating_cache;
import uc.algorithm;
import uc.iterator;
import uc.tag_invoke;

namespace uc {
    namespace sr = std::ranges;
    namespace sv = std::ranges::views;

    export
    template<sr::view V, typename P>
    class filter_view {
    private:
        V base_;
        movable_box<P> pred_;
        non_propagating_cache<sr::iterator_t<V>> cache_;

    public:
        constexpr filter_view()
        requires
            std::is_default_constructible_v<V> &&
            std::is_default_constructible_v<P>
        : base_{}, pred_{}, cache_{} {}

        constexpr filter_view(V v, P p) :
            base_{ std::move(v) },
            pred_{ std::move(p) },
            cache_{} {}

        constexpr V base() const&
        requires std::copy_constructible<V>
        {
            return base_;
        }
        constexpr V base() && {
            return std::move(base_);
        }
        constexpr const P& pred() const {
            return *pred_;
        }

        class iterator;
        class sentinel;

        constexpr iterator begin() {
            if constexpr (!sr::forward_range<V>) {
                return iterator{
                    this,
                    uc::find_if(
                        sr::begin(base_),
                        sr::end(base_),
                        std::ref(*pred_)
                    )
                };
            }
            else {
                if (!cache_.has_value()) {
                    cache_.emplace(
                        uc::find_if(
                            sr::begin(base_),
                            sr::end(base_),
                            std::ref(*pred_)
                        )
                    );
                }
                return iterator{ this, *cache_ };
            }
        }
        constexpr auto end() {
            if constexpr (sr::common_range<V>) {
                return iterator{ this, base_.end() };
            }
            else {
                return sentinel{ base_.end() };
            }
        }
    };

    template<sr::view V, typename P>
    class filter_view<V, P>::iterator {
    public:
        using value_type = sr::range_value_t<V>;
        using reference = sr::range_reference_t<V>;
        using difference_type =
            std::iter_difference_t<sr::iterator_t<V>>;

    private:
        using base_iterator = sr::iterator_t<V>;
        filter_view* parent_;
        base_iterator current_;

        constexpr decltype(auto)
        pred() noexcept {
            return *(parent_->pred_);
        }

    public:
        constexpr iterator() : parent_{ nullptr }, current_{} {}

        explicit constexpr iterator(
            filter_view* parent,
            base_iterator current
        ) :
            parent_{ parent }, 
            current_{ std::move(current) } {}

        constexpr const base_iterator& base() const& noexcept {
            return current_;
        }
        constexpr base_iterator base() && {
            return std::move(current_);
        }

        friend constexpr bool operator==(
            const iterator&,
            const iterator&
        ) = default;

        constexpr reference operator*() const {
            return *current_;
        }

        constexpr iterator& operator++() {
            ++current_;
            current_ = uc::find_if(
                current_, sr::end(parent_->base_),
                std::ref(*(parent_->pred_))
            );
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

        constexpr iterator& operator--()
        requires std::bidirectional_iterator<base_iterator>
        {
            while(
                (current_ != sr::begin(parent_->base_)) &&
                !std::invoke(*(parent_->pred_), uc::dec_and_read(current_))
            ) {}
            return *this;
        }
        constexpr iterator operator--(int)
        requires std::bidirectional_iterator<base_iterator>
        {
            const auto result{ *this };
            this->operator--();
            return result;
        }

        template<
            std::sentinel_for<iterator> S,
            indirect_non_regular_predicate<base_iterator> PP,
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
            PP pred,
            Dir dir
        ) {
            if constexpr (std::is_same_v<PP, skip_t>) {
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
                        return (
                            std::invoke(first.pred(), e) ?
                                std::invoke(pred, e) :
                                true
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

    template<sr::view V, typename P>
    class filter_view<V, P>::sentinel {
    private:
        using base_sentinel = sr::sentinel_t<V>;
        base_sentinel sen_;

    public:
        constexpr sentinel() : sen_{} {}

        constexpr sentinel(base_sentinel sen) :
            sen_{ std::move(sen) } {}

        constexpr const base_sentinel& base() const& noexcept {
            return sen_;
        }
        constexpr base_sentinel base() && {
            return std::move(sen_);
        }

        constexpr friend bool
        operator==(const iterator& lhs, const sentinel& rhs) {
            return lhs.base() == rhs.base();
        }
    };

    template<typename R, typename P>
    filter_view(R&&, P&&) -> filter_view<
        sv::all_t<R>, std::decay_t<P>
    >;

    struct filter_fn {
        template<typename P>
        class closure {
        private:
            P pred_;

        public:
            template<typename PP>
            constexpr closure(PP&& pred) : pred_{ std::forward<PP>(pred) } {}

            template<sr::viewable_range R>
            friend constexpr auto operator|(R&& r, closure&& clo) {
                return filter_view{
                    std::forward<R>(r),
                    std::forward<P>(clo.pred_)
                };
            }
        };

        template<typename P>
        constexpr closure<P> operator()(P&& pred) const {
            return closure<P>{ std::forward<P>(pred) };
        }
    };

    export
    constexpr filter_fn filter{};
}

namespace std::ranges {
    template<typename R, typename P>
    constexpr bool enable_view<uc::filter_view<R, P>>{ true };
}