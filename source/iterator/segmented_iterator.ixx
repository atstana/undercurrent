// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include <concepts>
#include <iterator>
#include <type_traits>

export module uc.iterator:segmented_iterator;

namespace uc {
    export
    template<typename I>
    struct segmented_iterator_traits{};

    export
    template<typename I>
    concept segmented_iterator =
        std::forward_iterator<I> &&
        requires(
            const I& i,
            typename segmented_iterator_traits<I>::segment_iterator seg,
            typename segmented_iterator_traits<I>::local_iterator local
        ) {
            { segmented_iterator_traits<I>::local(i) }
                -> std::same_as<decltype(local)>;
            requires std::forward_iterator<decltype(local)>;
            requires (
                std::bidirectional_iterator<I> ?
                std::bidirectional_iterator<decltype(local)> :
                true
            );

            requires std::is_same_v<
                std::iter_value_t<I>,
                std::iter_value_t<decltype(local)>
            >;
            requires std::is_same_v<
                std::iter_reference_t<I>,
                std::iter_reference_t<decltype(local)>
            >;
            requires std::is_same_v<
                std::iter_rvalue_reference_t<I>,
                std::iter_rvalue_reference_t<decltype(local)>
            >;

            { segmented_iterator_traits<I>::segment(i) }
                -> std::same_as<decltype(seg)>;
            { ++seg } -> std::same_as<decltype(seg)&>;
            { segmented_iterator_traits<I>::begin(seg) }
                -> std::same_as<decltype(local)>;
            { segmented_iterator_traits<I>::end(seg) }
                -> std::same_as<decltype(local)>;
            requires (
                std::bidirectional_iterator<I> ?
                requires { { --seg } -> std::same_as<decltype(seg)&>; } :
                true
            );
            requires std::regular<decltype(seg)>;

            { segmented_iterator_traits<I>::compose(seg, local) }
                -> std::same_as<I>;
        };
}
