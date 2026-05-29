// Copyright (c) 2026 atstana
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include <cstddef>
#include <iterator>

export module uc.dev:chunked_iterator;
import uc.iterator;

namespace uc {
    export
    template<typename T, std::size_t ChunkSize>
    class chunked_iterator {
    public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using iterator_concept = std::bidirectional_iterator_tag;

        struct chunk_ref {
            T* data_{};
            std::size_t size_{};
            std::size_t chunk_id_{};

            chunk_ref& operator++() { ++chunk_id_; return *this; }
            chunk_ref& operator--() { --chunk_id_; return *this; }
            friend bool operator==(const chunk_ref&, const chunk_ref&) = default;
        };

    private:
        T* data_{};
        std::size_t size_{};
        std::size_t cur_id_{};

    public:
        chunked_iterator() = default;

        template<std::size_t N>
        explicit chunked_iterator(T (&data)[N], std::size_t pos = 0) :
            data_{ data }, size_{ N }, cur_id_{ pos } {}

        chunked_iterator(T* data, std::size_t size, std::size_t pos) :
            data_{ data }, size_{ size }, cur_id_{ pos } {}

        T& operator*()  const { return data_[cur_id_]; }
        T* operator->() const { return data_ + cur_id_; }

        chunked_iterator& operator++() { ++cur_id_; return *this; }
        chunked_iterator  operator++(int) { auto t = *this; ++cur_id_; return t; }
        chunked_iterator& operator--() { --cur_id_; return *this; }
        chunked_iterator  operator--(int) { auto t = *this; --cur_id_; return t; }

        friend bool operator==(
            const chunked_iterator&,
            const chunked_iterator&
        ) = default;

        chunk_ref chunk() const {
            return { data_, size_, cur_id_ / ChunkSize };
        }
        T* local() const { return data_ + cur_id_; }
    };

    template<typename T, std::size_t ChunkSize>
    struct segmented_iterator_traits<chunked_iterator<T, ChunkSize>> {
        using iterator = chunked_iterator<T, ChunkSize>;
        using local_iterator = T*;
        using segment_iterator = typename iterator::chunk_ref;

        static local_iterator local(const iterator& i) {
            return i.local();
        }
        static segment_iterator segment(const iterator& i) {
            return i.chunk();
        }
        static local_iterator begin(const segment_iterator& s) {
            return s.data_ + ChunkSize * s.chunk_id_;
        }
        static local_iterator end(const segment_iterator& s) {
            auto cap = ChunkSize * (s.chunk_id_ + 1);
            return s.data_ + (cap < s.size_ ? cap : s.size_);
        }
        static iterator compose(
            const segment_iterator& s, local_iterator local
        ) {
            return iterator{
                s.data_, s.size_,
                static_cast<std::size_t>(local - s.data_)
            };
        }
    };
}
