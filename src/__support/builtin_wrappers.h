//===--Convenient template for builtins -------------------------*- C++ -*-===//
//             (Count Lead Zeroes) and (Count Trailing Zeros)
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_SRC_SUPPORT_BUILTIN_WRAPPERS_H
#define LLVM_LIBC_SRC_SUPPORT_BUILTIN_WRAPPERS_H

#include "named_pair.h"
#include "src/__support/CPP/type_traits.h"

namespace __llvm_libc {

// The following overloads are matched based on what is accepted by
// __builtin_clz/ctz* rather than using the exactly-sized aliases from stdint.h.
// This way, we can avoid making any assumptions about integer sizes and let the
// compiler match for us.
namespace __internal {

template <typename T> static inline int correct_zero(T val, int bits) {
  if (val == T(0))
    return sizeof(T(0)) * 8;
  else
    return bits;
}

template <typename T> static inline int clz(T val);
template <> inline int clz<unsigned int>(unsigned int val) {
  return __builtin_clz(val);
}
template <> inline int clz<unsigned long int>(unsigned long int val) {
  return __builtin_clzl(val);
}
template <> inline int clz<unsigned long long int>(unsigned long long int val) {
  return __builtin_clzll(val);
}

template <typename T> static inline int ctz(T val);
template <> inline int ctz<unsigned int>(unsigned int val) {
  return __builtin_ctz(val);
}
template <> inline int ctz<unsigned long int>(unsigned long int val) {
  return __builtin_ctzl(val);
}
template <> inline int ctz<unsigned long long int>(unsigned long long int val) {
  return __builtin_ctzll(val);
}
} // namespace __internal

template <typename T> static inline int safe_ctz(T val) {
  return __internal::correct_zero(val, __internal::ctz(val));
}

template <typename T> static inline int unsafe_ctz(T val) {
  return __internal::ctz(val);
}

template <typename T> static inline int safe_clz(T val) {
  return __internal::correct_zero(val, __internal::clz(val));
}

template <typename T> static inline int unsafe_clz(T val) {
  return __internal::clz(val);
}

// Add with carry
DEFINE_NAMED_PAIR_TEMPLATE(SumCarry, sum, carry);

template <typename T>
inline constexpr cpp::enable_if_t<
    cpp::is_integral_v<T> && cpp::is_unsigned_v<T>, SumCarry<T>>
add_with_carry(T a, T b, T carry_in) {
  T tmp = a + carry_in;
  T sum = b + tmp;
  T carry_out = (sum < b) || (tmp < a);
  return {sum, carry_out};
}

#if __has_builtin(__builtin_addc)
// https://clang.llvm.org/docs/LanguageExtensions.html#multiprecision-arithmetic-builtins

template <>
inline SumCarry<unsigned char>
add_with_carry<unsigned char>(unsigned char a, unsigned char b,
                              unsigned char carry_in) {
  SumCarry<unsigned char> result{0, 0};
  result.sum = __builtin_addcb(a, b, carry_in, &result.carry);
  return result;
}

template <>
inline SumCarry<unsigned short>
add_with_carry<unsigned short>(unsigned short a, unsigned short b,
                               unsigned short carry_in) {
  SumCarry<unsigned short> result{0, 0};
  result.sum = __builtin_addcs(a, b, carry_in, &result.carry);
  return result;
}

template <>
inline SumCarry<unsigned int>
add_with_carry<unsigned int>(unsigned int a, unsigned int b,
                             unsigned int carry_in) {
  SumCarry<unsigned int> result{0, 0};
  result.sum = __builtin_addc(a, b, carry_in, &result.carry);
  return result;
}

template <>
inline SumCarry<unsigned long>
add_with_carry<unsigned long>(unsigned long a, unsigned long b,
                              unsigned long carry_in) {
  SumCarry<unsigned long> result{0, 0};
  result.sum = __builtin_addcl(a, b, carry_in, &result.carry);
  return result;
}

template <>
inline SumCarry<unsigned long long>
add_with_carry<unsigned long long>(unsigned long long a, unsigned long long b,
                                   unsigned long long carry_in) {
  SumCarry<unsigned long long> result{0, 0};
  result.sum = __builtin_addcll(a, b, carry_in, &result.carry);
  return result;
}

#endif // __has_builtin(__builtin_addc)

// Subtract with borrow
DEFINE_NAMED_PAIR_TEMPLATE(DiffBorrow, diff, borrow);

template <typename T>
inline constexpr cpp::enable_if_t<
    cpp::is_integral_v<T> && cpp::is_unsigned_v<T>, DiffBorrow<T>>
sub_with_borrow(T a, T b, T borrow_in) {
  T tmp = a - b;
  T diff = tmp - borrow_in;
  T borrow_out = (diff > tmp) || (tmp > a);
  return {diff, borrow_out};
}

#if __has_builtin(__builtin_subc)
// https://clang.llvm.org/docs/LanguageExtensions.html#multiprecision-arithmetic-builtins

template <>
inline DiffBorrow<unsigned char>
sub_with_borrow<unsigned char>(unsigned char a, unsigned char b,
                               unsigned char borrow_in) {
  DiffBorrow<unsigned char> result{0, 0};
  result.diff = __builtin_subcb(a, b, borrow_in, &result.borrow);
  return result;
}

template <>
inline DiffBorrow<unsigned short>
sub_with_borrow<unsigned short>(unsigned short a, unsigned short b,
                                unsigned short borrow_in) {
  DiffBorrow<unsigned short> result{0, 0};
  result.diff = __builtin_subcs(a, b, borrow_in, &result.borrow);
  return result;
}

template <>
inline DiffBorrow<unsigned int>
sub_with_borrow<unsigned int>(unsigned int a, unsigned int b,
                              unsigned int borrow_in) {
  DiffBorrow<unsigned int> result{0, 0};
  result.diff = __builtin_subc(a, b, borrow_in, &result.borrow);
  return result;
}

template <>
inline DiffBorrow<unsigned long>
sub_with_borrow<unsigned long>(unsigned long a, unsigned long b,
                               unsigned long borrow_in) {
  DiffBorrow<unsigned long> result{0, 0};
  result.diff = __builtin_subcl(a, b, borrow_in, &result.borrow);
  return result;
}

template <>
inline DiffBorrow<unsigned long long>
sub_with_borrow<unsigned long long>(unsigned long long a, unsigned long long b,
                                    unsigned long long borrow_in) {
  DiffBorrow<unsigned long long> result{0, 0};
  result.diff = __builtin_subcll(a, b, borrow_in, &result.borrow);
  return result;
}

#endif // __has_builtin(__builtin_subc)

} // namespace __llvm_libc

#endif // LLVM_LIBC_SRC_SUPPORT_BUILTIN_WRAPPERS_H
