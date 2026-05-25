// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

export module uc.adaptors:take_while_view;
import :misc;
import :movable_box;
import uc.algorithm;
import uc.iterator;
import uc.tag_invoke;

namespace uc {
    namespace sr = std::ranges;
    namespace sv = std::ranges::views;

    export
    template<sr::view V, typename P>
    class take_while_view {
    private:
        V base_;
        movable_box<P> pred_;

    public:
        constexpr take_while_view()
        requires
            std::is_default_constructible_v<V> &&
            std::is_default_constructible_v<P>
        : base_{}, pred_{} {}

        constexpr take_while_view(V v, P p) :
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

        template<bool IsConst>
        class sentinel;

        constexpr auto begin()
        requires (!simple_view<V>)
        {
            return sr::begin(base_);
        }
        constexpr auto begin() const
        requires
            sr::range<const V> &&
            std::indirect_unary_predicate<
                const P,
                sr::iterator_t<const V>
            >
        {
            return sr::begin(base_);
        }

        constexpr sentinel<false> end()
        requires (!simple_view<V>)
        {
            return sentinel<false>{
                std::addressof(*pred_), sr::end(base_)
            };
        }
        constexpr sentinel<true> end() const
        requires
            sr::range<const V> &&
            std::indirect_unary_predicate<
                const P,
                sr::iterator_t<const V>
            >
        {
            return sentinel<true>{
                std::addressof(*pred_), sr::end(base_)
            };
        }
    };

    template<sr::view V, typename P>
    template<bool IsConst>
    class take_while_view<V, P>::sentinel {
    private:
        using base_type = maybe_const_t<IsConst, V>;
        using base_iterator = sr::iterator_t<base_type>;
        using base_sentinel = sr::sentinel_t<base_type>;
        using predicate = maybe_const_t<IsConst, P>;

        predicate* pred_;
        base_sentinel sen_;

    public:
        constexpr sentinel() : pred_{ nullptr }, sen_{}{}

        constexpr sentinel(predicate* pred, base_sentinel sen) :
            pred_{ pred },
            sen_{ std::move(sen) } {}

        constexpr const base_sentinel& base() const& noexcept {
            return sen_;
        }
        constexpr base_sentinel base() && {
            return std::move(sen_);
        }

        constexpr const predicate& pred() const noexcept {
            return *pred_;
        }

        constexpr friend bool
        operator==(const base_iterator& lhs, const sentinel& rhs) {
            return ((lhs == rhs.base()) || !std::invoke(*(rhs.pred_), *lhs));
        }

        template<indirect_non_regular_predicate<base_iterator> PP>
        friend constexpr advance_while_result<base_iterator>
        tag_invoke(
            tag_t<uc::advance_while>,
            base_iterator first,
            sentinel last,
            PP pred,
            forward_tag tag
        ) {
            if constexpr (std::is_same_v<PP, skip_t>) {
                auto pos = uc::find_if_not(
                    std::move(first),
                    std::move(last).base(),
                    std::ref(*(last.pred_))
                );
                return { std::move(pos), true };
            }
            else {
                bool tw_flag{ false };
                auto [pos, flag] = uc::advance_while(
                    std::move(first),
                    std::move(last).base(),
                    [&, pred = std::move(pred)]
                    (std::iter_reference_t<base_iterator> e) {
                        if (std::invoke(*(last.pred_), e)) {
                            return std::invoke(pred, e);
                        }
                        else {
                            tw_flag = true;
                            return false;
                        }
                    }
                );
                return { std::move(pos), flag || tw_flag };
            }
        }
    };

    template<typename R, typename P>
    take_while_view(R&&, P&&) -> take_while_view<
        sv::all_t<R>, std::decay_t<P>
    >;

    struct take_while_fn {
        template<typename P>
        class closure {
        private:
            P pred_;

        public:
            template<typename PP>
            constexpr closure(PP&& pred) : pred_{ std::forward<PP>(pred) } {}

            template<sr::viewable_range R>
            friend constexpr auto operator|(R&& r, closure&& clo) {
                return take_while_view{
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
    constexpr take_while_fn take_while{};
}

namespace std::ranges {
    template<typename R, typename P>
    constexpr bool enable_view<uc::take_while_view<R, P>>{ true };
}