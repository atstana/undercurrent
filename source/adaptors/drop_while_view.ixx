// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include <functional>
#include <iterator>
#include <ranges>
#include <utility>

export module uc.adaptors:drop_while_view;
import :movable_box;
import :non_propagating_cache;
import uc.algorithm;
import uc.iterator;

namespace uc {
    namespace sr = std::ranges;
    namespace sv = std::ranges::views;

    export
    template<sr::view V, typename P>
    class drop_while_view {
    private:
        V base_;
        movable_box<P> pred_;
        non_propagating_cache<sr::iterator_t<V>> cache_;

    public:
        constexpr drop_while_view()
        requires
            std::is_default_constructible_v<V> &&
            std::is_default_constructible_v<P>
        : base_{}, pred_{} {}

        constexpr drop_while_view(V v, P p) :
            base_{ std::move(v) },
            pred_{ std::move(p) } {}

        constexpr V base() const&
        requires std::copy_constructible<V>
        {
            return base_;
        }
        constexpr V base() && {
            return std::move(base_);
        }
        constexpr const P& pred() const noexcept {
            return *pred_;
        }

        constexpr sr::iterator_t<V> begin() {
            if (!cache_.has_value()) {
                cache_.emplace(
                    uc::find_if_not(
                        sr::begin(base_),
                        sr::end(base_),
                        std::cref(pred())
                    )
                );
            }
            return *cache_;
        }
        constexpr sr::sentinel_t<V> end() {
            return sr::end(base_);
        }
    };

    template<typename R, typename P>
    drop_while_view(R&&, P&&) -> drop_while_view<
        sv::all_t<R>, std::decay_t<P>
    >;

    struct drop_while_fn {
        template<typename P>
        class closure {
        private:
            P pred_;

        public:
            template<typename PP>
            constexpr closure(PP&& pred) : pred_{ std::forward<PP>(pred) } {}

            template<sr::viewable_range R>
            friend constexpr auto operator|(R&& r, closure&& clo) {
                return drop_while_view{
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
    constexpr drop_while_fn drop_while{};
}

namespace std::ranges {
    template<typename R, typename P>
    constexpr bool enable_view<uc::drop_while_view<R, P>>{ true };

    template<typename R, typename P>
    constexpr bool enable_borrowed_range<uc::drop_while_view<R, P>>{
        std::ranges::borrowed_range<R>
    };
}