//===-- Format string parser for scanf -------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_SRC_STDIO_SCANF_CORE_PARSER_H
#define LLVM_LIBC_SRC_STDIO_SCANF_CORE_PARSER_H

#include "src/__support/arg_list.h"
#include "src/stdio/scanf_core/core_structs.h"
#include "src/stdio/scanf_core/scanf_config.h"

#include <stddef.h>

namespace __llvm_libc {
namespace scanf_core {

class Parser {
  const char *__restrict str;

  size_t cur_pos = 0;
  internal::ArgList args_cur;

#ifndef LLVM_LIBC_SCANF_DISABLE_INDEX_MODE
  // args_start stores the start of the va_args, which is used when a previous
  // argument is needed. In that case, we have to read the arguments from the
  // beginning since they don't support reading backwards.
  internal::ArgList args_start;
  size_t args_index = 1;
#endif // LLVM_LIBC_SCANF_DISABLE_INDEX_MODE

public:
#ifndef LLVM_LIBC_SCANF_DISABLE_INDEX_MODE
  Parser(const char *__restrict new_str, internal::ArgList &args)
      : str(new_str), args_cur(args), args_start(args) {}
#else
  Parser(const char *__restrict new_str, internal::ArgList &args)
      : str(new_str), args_cur(args) {}
#endif // LLVM_LIBC_SCANF_DISABLE_INDEX_MODE

  // get_next_section will parse the format string until it has a fully
  // specified format section. This can either be a raw format section with no
  // conversion, or a format section with a conversion that has all of its
  // variables stored in the format section.
  FormatSection get_next_section();

private:
  // parse_length_modifier parses the length modifier inside a format string. It
  // assumes that str[*local_pos] is inside a format specifier. It returns a
  // LengthModifier with the length modifier it found. It will advance local_pos
  // after the format specifier if one is found.
  LengthModifier parse_length_modifier(size_t *local_pos);

  // get_next_arg_value gets the next value from the arg list as type T.
  template <class T> T inline get_next_arg_value() {
    return args_cur.next_var<T>();
  }

  //----------------------------------------------------
  // INDEX MODE ONLY FUNCTIONS AFTER HERE:
  //----------------------------------------------------

#ifndef LLVM_LIBC_SCANF_DISABLE_INDEX_MODE

  // parse_index parses the index of a value inside a format string. It
  // assumes that str[*local_pos] points to character after a '%' or '*', and
  // returns 0 if there is no closing $, or if it finds no number. If it finds a
  // number, it will move local_pos past the end of the $, else it will not move
  // local_pos.
  size_t parse_index(size_t *local_pos);

  // get_arg_value gets the value from the arg list at index (starting at 1).
  // This may require parsing the format string. An index of 0 is interpreted as
  // the next value.
  template <class T> T inline get_arg_value(size_t index) {
    if (!(index == 0 || index == args_index))
      args_to_index(index);

    ++args_index;
    return get_next_arg_value<T>();
  }

  // the ArgList can only return the next item in the list. This function is
  // used in index mode when the item that needs to be read is not the next one.
  // It moves cur_args to the index requested so the the appropriate value may
  // be read. This may involve parsing the format string, and is in the worst
  // case an O(n^2) operation.
  void args_to_index(size_t index);

#endif // LLVM_LIBC_SCANF_DISABLE_INDEX_MODE
};

} // namespace scanf_core
} // namespace __llvm_libc

#endif // LLVM_LIBC_SRC_STDIO_SCANF_CORE_PARSER_H
