// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>

export module uc.dev:seg_stack;
import uc.iterator;

namespace uc {
    export
    template<typename T, std::size_t SegSize, bool EnableSeg>
    class seg_stack;

    export
    template<typename T, std::size_t SegSize, bool EnableSeg>
    class seg_stack_segment_iterator;

    export
    template<typename T, std::size_t SegSize, bool EnableSeg>
    class seg_stack_iterator;

    template<typename T, std::size_t SegSize, bool EnableSeg = true>
    class seg_stack {
        friend class seg_stack_iterator<T, SegSize, EnableSeg>;
        friend class seg_stack_segment_iterator<T, SegSize, EnableSeg>;

    private:
        using main_alloc = std::allocator<T*>;
        using sub_alloc = std::allocator<T>;

        T** main_array_;
        std::size_t main_array_size_;
        std::size_t num_subs_;
        std::size_t num_elems_;

        static T** allocate_main(std::size_t n) {
            return main_alloc{}.allocate(n);
        }
        static void deallocate_main(T** main_array, std::size_t n) {
            return main_alloc{}.deallocate(main_array, n);
        }
        static T* allocate_sub() {
            return sub_alloc{}.allocate(SegSize);
        }
        static void deallocate_sub(T* sub) {
            sub_alloc{}.deallocate(sub, SegSize);
        }

        static std::size_t calc_seg_id(std::size_t i) {
            return i / SegSize;
        }
        static std::size_t offset(std::size_t seg_id, std::size_t i) {
            return i - seg_id * SegSize;
        }
        std::size_t last_offset() {
            return offset(num_subs_ - 1, num_elems_);
        }
        void destroy_subs() {
            if (num_subs_) {
                std::size_t cur{};
                while(cur != (num_subs_ - 1)) {
                    std::destroy_n(main_array_[cur], SegSize);
                    deallocate_sub(main_array_[cur]);
                    ++cur;
                }
                std::destroy_n(main_array_[num_subs_ - 1], last_offset());
                deallocate_sub(main_array_[num_subs_ - 1]);
            }
        }
        void destroy_main() {
            destroy_subs();
            deallocate_main(main_array_, main_array_size_);
        }
        void expand_main(std::size_t delta) {
            if (delta) {
                T** new_main_array{ allocate_main(main_array_size_ + delta) };
                std::uninitialized_copy_n(
                    main_array_, num_subs_, new_main_array
                );
                deallocate_main(main_array_, main_array_size_);
                main_array_ = new_main_array;
                main_array_size_ += delta;
            }
        }
        void append_sub() {
            assert(main_array_);
            if (num_subs_ == main_array_size_) {
                expand_main(main_array_size_);
            }
            main_array_[num_subs_] = allocate_sub();
            ++num_subs_;
        }

        T* get_ptr(std::size_t i) {
            const auto seg_id{ calc_seg_id(i) };
            return main_array_[seg_id] + offset(seg_id, i);
        }
        const T* get_ptr(std::size_t i) const {
            const auto seg_id{ calc_seg_id(i) };
            return main_array_[seg_id] + offset(seg_id, i);
        }

    public:
        using iterator = seg_stack_iterator<T, SegSize, EnableSeg>;
        using segment_iterator = seg_stack_segment_iterator<T, SegSize, EnableSeg>;

        seg_stack() :
            main_array_{},
            main_array_size_{},
            num_subs_{},
            num_elems_{}{}

        seg_stack(const seg_stack&) = delete;
        seg_stack& operator=(const seg_stack&) = delete;

        ~seg_stack() {
            destroy_main();
        }

        template<std::convertible_to<T> U>
        void push(U&& u) {
            if (!main_array_) {
                main_array_ = allocate_main(1);
                main_array_size_ = 1;
                main_array_[0] = allocate_sub();
                num_subs_ = 1;
            }
            if (num_elems_ == capacity()) {
                append_sub();
            }
            std::construct_at(get_ptr(num_elems_), std::forward<U>(u));
            ++num_elems_;
        }

        T& operator[](std::size_t i) {
            assert(i < num_elems_);
            return *(get_ptr(i));
        }
        const T& operator[](std::size_t i) const {
            assert(i < num_elems_);
            return *(get_ptr(i));
        }

        std::size_t capacity() const {
            return num_subs_ * SegSize;
        }
        std::size_t size() const {
            return num_elems_;
        }

        iterator begin();
        iterator end();
    };

    template<typename T, std::size_t SegSize, bool EnableSeg>
    class seg_stack_segment_iterator {
    private:
        seg_stack<T, SegSize, EnableSeg>* parent_;
        std::size_t seg_id_;

    public:
        seg_stack_segment_iterator() : parent_{}, seg_id_{} {}
        seg_stack_segment_iterator(
            seg_stack<T, SegSize, EnableSeg>* parent,
            std::size_t seg_id
        ) :
            parent_{ parent },
            seg_id_{ seg_id } {}

        seg_stack_segment_iterator& operator++() {
            ++seg_id_;
            return *this;
        }
        seg_stack_segment_iterator& operator--() {
            --seg_id_;
            return *this;
        }

        friend bool operator==(
            const seg_stack_segment_iterator&,
            const seg_stack_segment_iterator&
        ) = default;

        T* begin() const {
            if (seg_id_ == parent_->num_subs_) {
                return nullptr;
            }
            return parent_->main_array_[seg_id_];
        }
        T* end() const {
            if (seg_id_ == parent_->num_subs_) {
                return nullptr;
            }
            return parent_->main_array_[seg_id_] + SegSize;
        }

        std::size_t elem_id(T* local) const {
            assert((begin() <= local) && (local < end()));
            return (seg_id_ * SegSize) + (local - begin());
        }
        seg_stack<T, SegSize, EnableSeg>* parent() const {
            return parent_;
        }
    };

    template<typename T, std::size_t SegSize, bool EnableSeg>
    class seg_stack_iterator {
    private:
        seg_stack<T, SegSize, EnableSeg>* parent_;
        std::size_t elem_id_;

    public:
        using value_type = T;
        using reference = T&;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;
        using iterator_concept = std::bidirectional_iterator_tag;

        seg_stack_iterator() : parent_{}, elem_id_{} {}
        seg_stack_iterator(
            seg_stack<T, SegSize, EnableSeg>* parent,
            std::size_t elem_id
        ) :
            parent_{ parent },
            elem_id_{ elem_id } {}

        T& operator*() const {
            return (*parent_)[elem_id_];
        }

        seg_stack_iterator& operator++() {
            ++elem_id_;
            return *this;
        }
        seg_stack_iterator operator++(int) {
            auto result{ *this };
            this->operator++();
            return result;
        }
        seg_stack_iterator& operator--() {
            --elem_id_;
            return *this;
        }
        seg_stack_iterator operator--(int) {
            auto result{ *this };
            this->operator--();
            return result;
        }

        friend bool operator==(
            const seg_stack_iterator&,
            const seg_stack_iterator&
        ) = default;

        T* local() const {
            const auto seg_id{ elem_id_ / SegSize };
            if (seg_id == parent_->num_subs_) {
                return nullptr;
            }
            return parent_->main_array_[seg_id] + (elem_id_ - seg_id * SegSize);
        }

        seg_stack_segment_iterator<T, SegSize, EnableSeg> segment() const {
            return seg_stack_segment_iterator<T, SegSize, EnableSeg>{
                parent_, elem_id_ / SegSize
            };
        }
    };

    template<typename T, std::size_t SegSize, bool EnableSeg>
    typename seg_stack<T, SegSize, EnableSeg>::iterator
    seg_stack<T, SegSize, EnableSeg>::begin() {
        return iterator{ this, 0 };
    }

    template<typename T, std::size_t SegSize, bool EnableSeg>
    typename seg_stack<T, SegSize, EnableSeg>::iterator
    seg_stack<T, SegSize, EnableSeg>::end() {
        return iterator{ this, num_elems_ };
    }

    template<typename T, std::size_t SegSize>
    struct segmented_iterator_traits<seg_stack_iterator<T, SegSize, true>> {
        using iterator         = seg_stack_iterator<T, SegSize, true>;
        using local_iterator   = T*;
        using segment_iterator = seg_stack_segment_iterator<T, SegSize, true>;

        static local_iterator local(const iterator& i) {
            return i.local();
        }
        static segment_iterator segment(const iterator& i) {
            return i.segment();
        }
        static local_iterator begin(const segment_iterator& s) {
            return s.begin();
        }
        static local_iterator end(const segment_iterator& s) {
            return s.end();
        }
        static iterator compose(const segment_iterator& s, local_iterator l) {
            if (l == nullptr) {
                return iterator{ s.parent(), s.parent()->size() };
            }
            return iterator{ s.parent(), s.elem_id(l) };
        }
    };
}
