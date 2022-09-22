//===-- Unittests for sysconf ---------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/unistd/sysconf.h"
#include "utils/UnitTest/Test.h"

#include <unistd.h>

TEST(LlvmLibcSysconfTest, PagesizeTest) {
  long pagesize = __llvm_libc::sysconf(_SC_PAGESIZE);
  ASSERT_GT(pagesize, 0l);
}
