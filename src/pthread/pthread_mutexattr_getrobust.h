//===-- Implementation header for pthread_mutexattr_getrobust ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_SRC_PTHREAD_PTHREAD_MUTEXATTR_GETROBUST_H
#define LLVM_LIBC_SRC_PTHREAD_PTHREAD_MUTEXATTR_GETROBUST_H

#include <pthread.h>

namespace __llvm_libc {

int pthread_mutexattr_getrobust(const pthread_mutexattr_t *__restrict attr,
                                int *__restrict robust);

} // namespace __llvm_libc

#endif // LLVM_LIBC_SRC_PTHREAD_PTHREAD_MUTEXATTR_GETROBUST_H
