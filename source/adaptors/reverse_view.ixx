// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include <concepts>
#include <iterator>
#include <ranges>
#include <type_traits>

export module uc.adaptors:reverse_view;
import :misc;
import :non_propagating_cache;
import uc.iterator;

namespace uc {
    namespace sr = std::ranges;
    namespace sv = std::ranges::views;

    export
    template<sr::view V>
    requires sr::bidirectional_range<V>
    class reverse_view {
    private:
        V base_;
        non_propagating_cache<sr::iterator_t<V>> cache_;

    public:
        constexpr reverse_view()
        requires std::is_default_constructible_v<V>
        : base_{}, cache_{} {}

        explicit constexpr reverse_view(V v) :
            base_{ std::move(v) },
            cache_{} {}

        constexpr V base() const&
        requires std::copy_constructible<V>
        {
            return base_;
        }
        constexpr V base() && {
            return std::move(base_);
        }

        constexpr auto begin() {
            if (!cache_.has_value()) {
                cache_.emplace(uc::next(sr::begin(base_), sr::end(base_)));
            }
            return uc::make_iterator_reversed(*cache_);
        }
        constexpr auto begin()
        requires sr::common_range<V>
        {
            return uc::make_iterator_reversed(sr::end(base_));
        }
        constexpr auto begin() const
        requires sr::common_range<const V>
        {
            return uc::make_iterator_reversed(sr::end(base_));
        }

        constexpr auto end() {
            return uc::make_iterator_reversed(sr::begin(base_));
        }
        constexpr auto end() const
        requires sr::common_range<const V>
        {
            return uc::make_iterator_reversed(sr::begin(base_));
        }
    };

    template<typename R>
    reverse_view(R&&) -> reverse_view<sv::all_t<R>>;

    template<typename T>
    constexpr bool is_a_reverse_view_v{ false };

    template<typename V>
    constexpr bool is_a_reverse_view_v<reverse_view<V>>{ true };

    struct reverse_fn {
        template<sr::viewable_range R>
        requires sr::bidirectional_range<R>
        friend constexpr auto operator|(R&& r, reverse_fn fact) {
            if constexpr (is_a_reverse_view_v<std::decay_t<R>>) {
                return std::forward<R>(r).base();
            }
            else {
                return reverse_view{ std::forward<R>(r) };
            }
        }
    };

    export
    constexpr reverse_fn reverse{};
}

namespace std::ranges {
    template<typename R>
    constexpr bool enable_view<uc::reverse_view<R>>{ true };

    template<typename R>
    constexpr bool enable_borrowed_range<uc::reverse_view<R>>{
        std::ranges::borrowed_range<R>
    };
}