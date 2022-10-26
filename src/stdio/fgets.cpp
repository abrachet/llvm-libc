//===-- Implementation of fgets -------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/stdio/fgets.h"
#include "src/__support/File/file.h"

#include <stddef.h>
#include <stdio.h>

namespace __llvm_libc {

LLVM_LIBC_FUNCTION(char *, fgets,
                   (char *__restrict str, int count,
                    ::FILE *__restrict raw_stream)) {
  if (count < 1)
    return nullptr;

  unsigned char c = '\0';
  auto stream = reinterpret_cast<__llvm_libc::File *__restrict>(raw_stream);
  stream->lock();

  // i is an int because it's frequently compared to count, which is also int.
  int i = 0;

  for (; i < (count - 1) && c != '\n'; ++i) {
    size_t r = stream->read_unlocked(&c, 1);
    if (r != 1)
      break;
    str[i] = c;
  }

  bool has_error = stream->error_unlocked();
  bool has_eof = stream->iseof_unlocked();
  stream->unlock();

  // If the requested read size makes no sense, an error occured, or no bytes
  // were read due to an EOF, then return nullptr and don't write the null byte.
  if (has_error || (i == 0 && has_eof))
    return nullptr;

  str[i] = '\0';
  return str;
}

} // namespace __llvm_libc
