//===-- Implementation header for wait --------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_SRC_SYS_WAIT_WAIT_H
#define LLVM_LIBC_SRC_SYS_WAIT_WAIT_H

#include <sys/wait.h>

namespace __llvm_libc {

pid_t wait(int *waitstatus);

} // namespace __llvm_libc

#endif // LLVM_LIBC_SRC_SYS_WAIT_WAIT_H
