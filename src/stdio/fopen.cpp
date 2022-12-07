//===-- Implementation of fopen -------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/stdio/fopen.h"
#include "src/__support/File/file.h"

#include <errno.h>
#include <stdio.h>

namespace __llvm_libc {

LLVM_LIBC_FUNCTION(::FILE *, fopen,
                   (const char *__restrict name, const char *__restrict mode)) {
  auto result = __llvm_libc::openfile(name, mode);
  if (!result.has_value()) {
    errno = result.error();
    return nullptr;
  }
  return reinterpret_cast<::FILE *>(result.value());
}

} // namespace __llvm_libc
