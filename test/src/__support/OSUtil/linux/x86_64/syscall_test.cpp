//===-- Unittests for x86_64 syscalls -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/__support/CPP/functional.h"
#include "src/__support/OSUtil/syscall.h"
#include "utils/UnitTest/Test.h"

TEST(LlvmLibcX86_64_SyscallTest, APITest) {
  // We only do a signature test here. Actual functionality tests are
  // done by the unit tests of the syscall wrappers like mmap.

  using __llvm_libc::cpp::function;

  function<long(long)> f([](long n) { return __llvm_libc::syscall_impl(n); });
  function<long(long, long)> f1(
      [](long n, long a1) { return __llvm_libc::syscall_impl(n, a1); });
  function<long(long, long, long)> f2([](long n, long a1, long a2) {
    return __llvm_libc::syscall_impl(n, a1, a2);
  });
  function<long(long, long, long, long)> f3(
      [](long n, long a1, long a2, long a3) {
        return __llvm_libc::syscall_impl(n, a1, a2, a3);
      });
  function<long(long, long, long, long, long)> f4(
      [](long n, long a1, long a2, long a3, long a4) {
        return __llvm_libc::syscall_impl(n, a1, a2, a3, a4);
      });
  function<long(long, long, long, long, long, long)> f5(
      [](long n, long a1, long a2, long a3, long a4, long a5) {
        return __llvm_libc::syscall_impl(n, a1, a2, a3, a4, a5);
      });
  function<long(long, long, long, long, long, long, long)> f6(
      [](long n, long a1, long a2, long a3, long a4, long a5, long a6) {
        return __llvm_libc::syscall_impl(n, a1, a2, a3, a4, a5, a6);
      });

  function<long(long, void *)> not_long_type(
      [](long n, void *a1) { return __llvm_libc::syscall_impl(n, a1); });
}
