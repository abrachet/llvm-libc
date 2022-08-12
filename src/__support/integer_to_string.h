//===-- Utilities to convert integral values to string ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_SRC_SUPPORT_INTEGER_TO_STRING_H
#define LLVM_LIBC_SRC_SUPPORT_INTEGER_TO_STRING_H

#include "src/__support/CPP/ArrayRef.h"
#include "src/__support/CPP/StringView.h"
#include "src/__support/CPP/optional.h"
#include "src/__support/CPP/type_traits.h"

namespace __llvm_libc {

// Convert integer values to their string representation.
//
// Example usage:
//   int a = 1234567;
//
//   // Convert to hexadecimal string:
//   char hexbuf[IntegerToString::hex_bufsize<int>()];
//   auto str = IntegerToString::hex(
//       a, hexbuf, false /* generate upper case characters */);
//
//   // Convert to decimal string:
//   char decbuf[IntegerToString::dec_bufsize<int>()];
//   auto str = IntegerToString::dec(a, decbuf);
//
//   // Convert to octal string:
//   char octbuf[IntegerToString::oct_bufsize<int>(a)];
//   auto str = IntegerToString::dec(a, octbuf);
//
//   // Convert to binary string:
//   char binbuf[IntegerToString::bin_bufsize<int>(a)];
//   auto str = IntegerToString::bin(a, binbuf);
//
//   // Convert to base 30 string:
//   char b30buf[IntegerToString::bufsize<30, int>(a)];
//   auto str = IntegerToString::convert<30>(a, b30buf);
class IntegerToString {
  static cpp::StringView convert_uintmax(uintmax_t uval,
                                         cpp::MutableArrayRef<char> &buffer,
                                         bool lowercase,
                                         const uint8_t conv_base) {
    const char a = lowercase ? 'a' : 'A';

    size_t len = 0;

    size_t buffptr = buffer.size();
    if (uval == 0) {
      buffer[buffptr - 1] = '0';
      --buffptr;
    } else {
      for (; uval > 0; --buffptr, uval /= conv_base) {
        uintmax_t digit = (uval % conv_base);
        buffer[buffptr - 1] = digit < 10 ? digit + '0' : digit + a - 10;
      }
    }
    len = buffer.size() - buffptr;

    return cpp::StringView(buffer.data() + buffer.size() - len, len);
  }

  static cpp::StringView convert_intmax(intmax_t val,
                                        cpp::MutableArrayRef<char> &buffer,
                                        bool lowercase,
                                        const uint8_t conv_base) {
    if (val >= 0)
      return convert_uintmax(uintmax_t(val), buffer, lowercase, conv_base);
    uintmax_t uval = uintmax_t(-val);
    auto str_view = convert_uintmax(uval, buffer, lowercase, conv_base);
    size_t len = str_view.size();
    ++len;
    buffer[buffer.size() - len] = '-';
    return cpp::StringView(buffer.data() + buffer.size() - len, len);
  }

  static constexpr inline size_t floor_log_2(size_t num) {
    size_t i = 0;
    for (; num > 1; num /= 2) {
      ++i;
    }
    return i;
  }

public:
  // We size the string buffer for base 10 using an approximation algorithm:
  //
  //   size = ceil(sizeof(T) * 5 / 2)
  //
  // If sizeof(T) is 1, then size is 3 (actually need 3)
  // If sizeof(T) is 2, then size is 5 (actually need 5)
  // If sizeof(T) is 4, then size is 10 (actually need 10)
  // If sizeof(T) is 8, then size is 20 (actually need 20)
  // If sizeof(T) is 16, then size is 40 (actually need 39)
  //
  // NOTE: The ceil operation is actually implemented as
  //     floor(((sizeof(T) * 5) + 1)/2)
  // where floor operation is just integer division.
  //
  // This estimation grows slightly faster than the actual value, but the
  // overhead is small enough to tolerate. In the actual formula below, we
  // add an additional byte to accommodate the '-' sign in case of signed
  // integers.
  // For other bases, we approximate by rounding down to the nearest power of
  // two base, since the space needed is easy to calculate and it won't
  // overestimate by too much.
  template <uint8_t BASE, typename T> static constexpr size_t bufsize() {
    constexpr size_t BITS_PER_DIGIT = floor_log_2(BASE);
    constexpr size_t BUFSIZE_COMMON =
        ((sizeof(T) * 8 + (BITS_PER_DIGIT - 1)) / BITS_PER_DIGIT);
    constexpr size_t BUFSIZE_BASE10 = (sizeof(T) * 5 + 1) / 2;
    return (cpp::is_signed<T>() ? 1 : 0) +
           (BASE == 10 ? BUFSIZE_BASE10 : BUFSIZE_COMMON);
  }

  template <typename T> static constexpr size_t dec_bufsize() {
    return bufsize<10, T>();
  }

  template <typename T> static constexpr size_t hex_bufsize() {
    return bufsize<16, T>();
  }

  template <typename T> static constexpr size_t oct_bufsize() {
    return bufsize<8, T>();
  }

  template <typename T> static constexpr size_t bin_bufsize() {
    return bufsize<2, T>();
  }

  template <uint8_t BASE, typename T,
            cpp::enable_if_t<2 <= BASE && BASE <= 36 && cpp::is_integral_v<T>,
                             int> = 0>
  static cpp::optional<cpp::StringView>
  convert(T val, cpp::MutableArrayRef<char> buffer, bool lowercase = true) {
    if (buffer.size() < bufsize<BASE, T>())
      return cpp::optional<cpp::StringView>();
    if (cpp::is_signed_v<T>)
      return convert_intmax(intmax_t(val), buffer, lowercase, BASE);
    else
      return convert_uintmax(uintmax_t(val), buffer, lowercase, BASE);
  }

  template <typename T, cpp::enable_if_t<cpp::is_integral_v<T>, int> = 0>
  static cpp::optional<cpp::StringView> dec(T val,
                                            cpp::MutableArrayRef<char> buffer) {
    return convert<10>(val, buffer);
  }

  template <typename T, cpp::enable_if_t<cpp::is_integral_v<T>, int> = 0>
  static cpp::optional<cpp::StringView>
  hex(T val, cpp::MutableArrayRef<char> buffer, bool lowercase = true) {
    return convert<16>(val, buffer, lowercase);
  }

  template <typename T, cpp::enable_if_t<cpp::is_integral_v<T>, int> = 0>
  static cpp::optional<cpp::StringView> oct(T val,
                                            cpp::MutableArrayRef<char> buffer) {
    return convert<8>(val, buffer);
  }

  template <typename T, cpp::enable_if_t<cpp::is_integral_v<T>, int> = 0>
  static cpp::optional<cpp::StringView> bin(T val,
                                            cpp::MutableArrayRef<char> buffer) {
    return convert<2>(val, buffer);
  }
};

} // namespace __llvm_libc

#endif // LLVM_LIBC_SRC_SUPPORT_INTEGER_TO_STRING_H
