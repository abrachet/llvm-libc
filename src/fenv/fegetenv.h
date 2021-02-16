//===-- Implementation header for fegetenv ----------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_SRC_FENV_FEGETENV_H
#define LLVM_LIBC_SRC_FENV_FEGETENV_H

#include <fenv.h>

namespace __llvm_libc {

int fegetenv(fenv_t *);

} // namespace __llvm_libc

#endif // LLVM_LIBC_SRC_FENV_FEGETENV_H
