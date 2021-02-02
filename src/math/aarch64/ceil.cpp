//===-- Implementation of the ceil function for aarch64 -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/math/ceil.h"
#include "src/__support/common.h"

namespace __llvm_libc {

LLVM_LIBC_FUNCTION(double, ceil, (double x)) {
  double y;
  __asm__ __volatile__("ldr d0, %1\n"
                       "frintp d0, d0\n"
                       "str d0, %0\n"
                       : "=m"(y)
                       : "m"(x)
                       : "d0");
  return y;
}

} // namespace __llvm_libc
