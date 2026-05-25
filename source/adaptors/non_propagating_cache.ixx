// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include <cassert>
#include <concepts>
#include <memory>
#include <type_traits>

export module uc.adaptors:non_propagating_cache;

namespace uc {
    template<typename T>
    requires std::is_object_v<T>
    class non_propagating_cache {
    private:
        union { T value_; };
        bool has_value_;

    public:
        constexpr non_propagating_cache() noexcept
        : has_value_{ false } {}

        template<typename U>
        requires std::convertible_to<U, T>
        explicit constexpr non_propagating_cache(U&& u)
        noexcept(std::is_nothrow_constructible_v<T, U>)
        : value_{ std::forward<U>(u) }, has_value_{ true } {}

        constexpr non_propagating_cache(const non_propagating_cache& other)
        noexcept
        : has_value_{ false } {}

        constexpr non_propagating_cache(non_propagating_cache&& other)
        noexcept
        : has_value_{ false } { other.reset(); }

        constexpr non_propagating_cache&
        operator=(const non_propagating_cache& other)
        noexcept
        {
            if (this != &other) {
                reset();
            }
            return *this;
        }
        constexpr non_propagating_cache&
        operator=(non_propagating_cache&& other)
        noexcept
        {
            if (this != &other) {
                reset();
                other.reset();
            }
            return *this;
        }

        constexpr ~non_propagating_cache() {
            reset();
        }

        constexpr T& operator*() noexcept {
            assert(has_value_);
            return value_;
        }
        constexpr const T& operator*() const noexcept {
            assert(has_value_);
            return value_;
        }
        constexpr T* operator->() noexcept {
            assert(has_value_);
            return std::addressof(value_);
        }
        constexpr const T* operator->() const noexcept {
            assert(has_value_);
            return std::addressof(value_);
        }

        explicit constexpr operator bool() const noexcept {
            return has_value_;
        }

        constexpr bool has_value() const noexcept {
            return has_value_;
        }

        constexpr void reset() noexcept {
            if (has_value_) {
                value_.~T();
                has_value_ = false;
            }
        }
        template<typename... Args>
        constexpr T& emplace(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<T, Args...>)
        {
            reset();
            std::construct_at(std::addressof(value_), std::forward<Args>(args)...);
            has_value_ = true;
            return value_;
        }
        template<typename U, typename... Args>
        constexpr T& emplace(std::initializer_list<U> list, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<std::initializer_list<U>&, Args...>)
        {
            reset();
            std::construct_at(
                std::addressof(value_),
                list, std::forward<Args>(args)...
            );
            has_value_ = true;
            return value_;
        }
    };
}