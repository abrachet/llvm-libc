//===-- Implementation of putc --------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/stdio/putc.h"
#include "src/__support/File/file.h"

#include <stdio.h>

namespace __llvm_libc {

LLVM_LIBC_FUNCTION(int, putc, (int c, ::FILE *stream)) {
  unsigned char uc = static_cast<unsigned char>(c);
  size_t written = reinterpret_cast<__llvm_libc::File *>(stream)->write(&uc, 1);
  if (1 != written) {
    // The stream should be in an error state in this case.
    return EOF;
  }
  return 0;
}

} // namespace __llvm_libc
