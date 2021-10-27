//===-- Unittests for sinf ------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/__support/CPP/Array.h"
#include "src/__support/FPUtil/FPBits.h"
#include "src/__support/FPUtil/TestHelpers.h"
#include "src/math/sinf.h"
#include "test/src/math/sdcomp26094.h"
#include "utils/MPFRWrapper/MPFRUtils.h"
#include "utils/UnitTest/Test.h"
#include <math.h>

#include <errno.h>
#include <stdint.h>

using __llvm_libc::testing::sdcomp26094Values;
using FPBits = __llvm_libc::fputil::FPBits<float>;

namespace mpfr = __llvm_libc::testing::mpfr;

DECLARE_SPECIAL_CONSTANTS(float)

TEST(LlvmLibcSinfTest, SpecialNumbers) {
  errno = 0;

  EXPECT_FP_EQ(aNaN, __llvm_libc::sinf(aNaN));
  EXPECT_EQ(errno, 0);

  EXPECT_FP_EQ(0.0f, __llvm_libc::sinf(0.0f));
  EXPECT_EQ(errno, 0);

  EXPECT_FP_EQ(-0.0f, __llvm_libc::sinf(-0.0f));
  EXPECT_EQ(errno, 0);

  errno = 0;
  EXPECT_FP_EQ(aNaN, __llvm_libc::sinf(inf));
  EXPECT_EQ(errno, EDOM);

  errno = 0;
  EXPECT_FP_EQ(aNaN, __llvm_libc::sinf(negInf));
  EXPECT_EQ(errno, EDOM);
}

TEST(LlvmLibcSinfTest, InFloatRange) {
  constexpr uint32_t count = 1000000;
  constexpr uint32_t step = UINT32_MAX / count;
  for (uint32_t i = 0, v = 0; i <= count; ++i, v += step) {
    float x = float(FPBits(v));
    if (isnan(x) || isinf(x))
      continue;
    ASSERT_MPFR_MATCH(mpfr::Operation::Sin, x, __llvm_libc::sinf(x), 1.0);
  }
}

TEST(LlvmLibcSinfTest, SpecificBitPatterns) {
  float x = float(FPBits(uint32_t(0xc70d39a1)));
  EXPECT_MPFR_MATCH(mpfr::Operation::Sin, x, __llvm_libc::sinf(x), 1.0);
}

// For small values, sin(x) is x.
TEST(LlvmLibcSinfTest, SmallValues) {
  float x = float(FPBits(uint32_t(0x17800000)));
  float result = __llvm_libc::sinf(x);
  EXPECT_MPFR_MATCH(mpfr::Operation::Sin, x, result, 1.0);
  EXPECT_FP_EQ(x, result);

  x = float(FPBits(uint32_t(0x00400000)));
  result = __llvm_libc::sinf(x);
  EXPECT_MPFR_MATCH(mpfr::Operation::Sin, x, result, 1.0);
  EXPECT_FP_EQ(x, result);
}

// SDCOMP-26094: check sinf in the cases for which the range reducer
// returns values furthest beyond its nominal upper bound of pi/4.
TEST(LlvmLibcSinfTest, SDCOMP_26094) {
  for (uint32_t v : sdcomp26094Values) {
    float x = float(FPBits((v)));
    EXPECT_MPFR_MATCH(mpfr::Operation::Sin, x, __llvm_libc::sinf(x), 1.0);
  }
}
