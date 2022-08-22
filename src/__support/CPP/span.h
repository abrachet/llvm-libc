//===-- Standalone implementation std::span ---------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_LIBC_SRC_SUPPORT_CPP_SPAN_H
#define LLVM_LIBC_SRC_SUPPORT_CPP_SPAN_H

#include <stddef.h> // For size_t

#include "array.h"       // For array
#include "type_traits.h" // For remove_cv_t, enable_if_t, is_same_v, is_const_v

namespace __llvm_libc::cpp {

// A trimmed down implementation of std::span.
// Missing features:
// - No constant size spans (e.g. Span<int, 4>),
// - Only handle pointer like types, no fancy interators nor object overriding
//   the & operator,
// - No implicit type conversion (e.g. Span<B>, initialized with As where A
//   inherits from B),
// - No reverse iterators
template <typename T> class span {
public:
  using element_type = T;
  using value_type = remove_cv_t<T>;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using pointer = T *;
  using const_pointer = const T *;
  using reference = T &;
  using const_reference = const T &;
  using iterator = T *;

  static constexpr size_type dynamic_extent = -1;

  constexpr span() : span_data(nullptr), span_size(0) {}

  constexpr span(pointer first, size_type count)
      : span_data(first), span_size(count) {}

  constexpr span(pointer first, pointer end)
      : span_data(first), span_size(end - first) {}

  template <size_t N>
  constexpr span(element_type (&arr)[N]) : span_data(arr), span_size(N) {}

  template <size_t N>
  constexpr span(array<T, N> &arr)
      : span_data(arr.data()), span_size(arr.size()) {}

  template <typename U,
            cpp::enable_if_t<!cpp::is_const_v<U> && cpp::is_const_v<T> &&
                                 cpp::is_same_v<U, value_type>,
                             bool> = true>
  constexpr span(span<U> &s) : span(s.data(), s.size()) {}

  constexpr span(const span &s) = default;
  constexpr span &operator=(const span &s) = default;
  ~span() = default;
  constexpr reference operator[](size_type index) const {
    return data()[index];
  }
  constexpr iterator begin() const { return data(); }
  constexpr iterator end() const { return data() + size(); }
  constexpr reference front() const { return (*this)[0]; }
  constexpr reference back() const { return (*this)[size() - 1]; }
  constexpr pointer data() const { return span_data; }
  constexpr size_type size() const { return span_size; }
  constexpr size_type size_bytes() const { return sizeof(T) * size(); }
  constexpr bool empty() const { return size() == 0; }

  constexpr span<element_type> subspan(size_type offset,
                                       size_type count = dynamic_extent) const {
    return span<element_type>(data() + offset, count_to_size(offset, count));
  }

  constexpr span<element_type> first(size_type count) const {
    return subspan(0, count);
  }

  constexpr span<element_type> last(size_type count) const {
    return span<element_type>(data() + (size() - count), count);
  }

private:
  constexpr size_type count_to_size(size_type offset, size_type count) const {
    if (count == dynamic_extent) {
      return size() - offset;
    }
    return count;
  }

  T *span_data;
  size_t span_size;
};

} // namespace __llvm_libc::cpp

#endif /* LLVM_LIBC_SRC_SUPPORT_CPP_SPAN_H */
