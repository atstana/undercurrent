// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include <concepts>
#include <functional>
#include <iterator>
#include <type_traits>

export module uc.iterator:iteration;
import :misc;
import uc.concepts;
import uc.tag_invoke;

namespace uc {
    struct read_and_inc_fn{
        template<typename I>
        requires tag_invocable<read_and_inc_fn, I&>
        constexpr tag_invoke_result_t<read_and_inc_fn, I&>
        operator()(I& cur) const {
            return uc::tag_invoke(*this, cur);
        }

        template<std::input_iterator I>
        friend constexpr std::iter_reference_t<I>
        tag_invoke(read_and_inc_fn, I& cur) {
            std::iter_reference_t<I> result{ *cur };
            ++cur;
            return result;
        }
    };
    export
    constexpr read_and_inc_fn read_and_inc{};

    struct dec_and_read_fn{
        template<typename I>
        requires tag_invocable<dec_and_read_fn, I&>
        constexpr tag_invoke_result_t<dec_and_read_fn, I&>
        operator()(I& cur) const {
            return uc::tag_invoke(*this, cur);
        }

        template<std::bidirectional_iterator I>
        friend constexpr std::iter_reference_t<I>
        tag_invoke(dec_and_read_fn, I& cur) {
            --cur;
            return *cur;
        }
    };
    export
    constexpr dec_and_read_fn dec_and_read{};

    export
    struct backward_tag;
    export
    struct forward_tag{
        constexpr operator bool() const noexcept {
            return true;
        }
        constexpr backward_tag operator!() const noexcept;
    };
    export
    struct backward_tag {
        constexpr operator bool() const noexcept {
            return false;
        }
        constexpr forward_tag operator!() const noexcept;
    };

    constexpr backward_tag
    forward_tag::operator!() const noexcept {
        return backward_tag{};
    }
    constexpr forward_tag
    backward_tag::operator!() const noexcept {
        return forward_tag{};
    }

    export
    template<typename T>
    concept direction_tag =
        std::is_same_v<T, forward_tag> ||
        std::is_same_v<T, backward_tag>;

    export
    template<typename I, typename S, typename Dir>
    concept advancable =
        direction_tag<Dir> &&
        requires(Dir dir) {
            requires (
                dir ?
                (
                    std::input_iterator<I> &&
                    std::sentinel_for<S, I>
                ) : (
                    std::bidirectional_iterator<I> &&
                    std::is_same_v<I, S>
                )
            );
        };

    export
    struct skip_t {
        constexpr bool operator()(...) const noexcept {
            return true;
        }
    };
    export
    constexpr skip_t skip{};

    export
    template<typename I>
    struct advance_while_result{
        I pos;
        bool completion_flag;
    };

    export
    template<typename P, typename I>
    concept indirect_non_regular_predicate =
        std::indirectly_readable<I> &&
        non_regular_predicate<P, std::iter_reference_t<I>>;


    struct advance_while_fn{
        template<typename I, typename S, typename P, typename Dir = forward_tag>
        requires tag_invocable<advance_while_fn, I, S, P, Dir>
        constexpr tag_invoke_result_t<advance_while_fn, I, S, P, Dir>
        operator()(I&& first, S&& last, P&& pred, Dir&& dir = {}) const {
            return uc::tag_invoke(
                *this,
                std::forward<I>(first),
                std::forward<S>(last),
                std::forward<P>(pred),
                std::forward<Dir>(dir)
            );
        }

        template<
            std::input_iterator I,
            std::sentinel_for<I> S,
            indirect_non_regular_predicate<I> P,
            direction_tag Dir
        >
        requires advancable<I, S, Dir>
        friend constexpr advance_while_result<I>
        tag_invoke(advance_while_fn, I first, S last, P pred, Dir dir) {
            if constexpr (std::is_same_v<P, skip_t>) {
                if constexpr (dir) {
                    return {
                        std::ranges::next(
                            std::move(first), std::move(last)
                        ),
                        true
                    };
                }
                else {
                    return { std::move(first), true };
                }
            }
            else {
                if constexpr (dir) {
                    while(first != last) {
                        if (!std::invoke(pred, *first)) {
                            return { std::move(first), false };
                        }
                        ++first;
                    }
                    return { std::move(first), true };
                }
                else {
                    while(first != last) {
                        --last;
                        if (!std::invoke(pred, *last)) {
                            return { std::move(last), false };
                        }
                    }
                    return { std::move(last), true };
                }
            }
        }
    };
    export
    constexpr advance_while_fn advance_while{};

    struct next_fn{
        template<std::input_iterator I, std::sentinel_for<I> S>
        I operator()(I cur, S bound) const {
            return uc::advance_while(
                std::move(cur),
                std::move(bound),
                skip
            ).pos;
        }
    };
    export
    constexpr next_fn next{};
}