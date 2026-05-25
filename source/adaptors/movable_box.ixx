// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include <cassert>
#include <concepts>
#include <memory>
#include <optional>
#include <type_traits>

export module uc.adaptors:movable_box;

namespace uc {
    template<typename T>
    requires
        std::move_constructible<T> &&
        std::is_object_v<T>
    class movable_box {
    private:
        union { T value_; };
        bool has_value_;

    public:
        constexpr movable_box()
        noexcept(std::is_nothrow_default_constructible_v<T>)
        requires std::is_default_constructible_v<T>
        : value_{}, has_value_{ true } {}

        explicit constexpr movable_box(std::nullopt_t) noexcept
        : has_value_{ false } {}

        template<typename U>
        requires std::convertible_to<U, T>
        explicit constexpr movable_box(U&& u)
        noexcept(std::is_nothrow_constructible_v<T, U>)
        : value_{ std::forward<U>(u) }, has_value_{ true }{}

        constexpr movable_box(const movable_box& other)
        noexcept(std::is_nothrow_copy_constructible_v<T>)
        {
            if (other.has_value_) {
                std::construct_at(std::addressof(value_), *other);
                has_value_ = true;
            }
            else {
                has_value_ = false;
            }
        }
        constexpr movable_box(movable_box&& other)
        noexcept(std::is_nothrow_move_constructible_v<T>)
        {
            if (other.has_value_) {
                std::construct_at(std::addressof(value_), std::move(*other));
                has_value_ = true;
            }
            else {
                has_value_ = false;
            }
        }

        constexpr movable_box& operator=(const movable_box& other)
        noexcept(std::is_nothrow_copy_constructible_v<T>)
        requires std::copy_constructible<T>
        {
            if (this != &other) {
                if (other.has_value_) {
                    emplace(*other);
                }
                else {
                    reset();
                }
            }
            return *this;
        }
        constexpr movable_box& operator=(movable_box&& other)
        noexcept(std::is_nothrow_move_constructible_v<T>)
        {
            if (this != &other) {
                if (other.has_value_) {
                    emplace(std::move(*other));
                }
                else {
                    reset();
                }
            }
            return *this;
        }

        constexpr ~movable_box() {
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