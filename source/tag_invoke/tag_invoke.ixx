// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include <type_traits>

export module uc.tag_invoke;

namespace uc {
    namespace tag_invoke_detail {
        constexpr void tag_invoke(...) = delete;

        struct tag_invoke_fn {
            template<typename Tag, typename... Args>
            constexpr auto operator()(Tag tag, Args&&... args) const
                noexcept(noexcept(tag_invoke((Tag&&)tag, (Args&&)args...)))
                -> decltype(tag_invoke((Tag&&)tag, (Args&&)args...))
            {
                return tag_invoke((Tag&&)tag, (Args&&)args...);
            }
        };
    }

    inline namespace tag_invoke_export {
        export
        constexpr uc::tag_invoke_detail::tag_invoke_fn tag_invoke{};
    }

    export
    template<auto& T>
    using tag_t = std::decay_t<decltype(T)>;
    
    export
    template<typename Tag, typename... Args>
    concept tag_invocable = std::is_invocable_v<decltype(tag_invoke), Tag, Args...>;

    export
    template<typename Tag, typename... Args>
    concept nothrow_tag_invocable =
        tag_invocable<Tag, Args...> &&
        std::is_nothrow_invocable_v<decltype(tag_invoke), Tag, Args...>;

    export
    template<typename Tag, typename... Args>
    using tag_invoke_result_t = std::invoke_result_t<decltype(tag_invoke), Tag, Args...>;
}