//===-- Single-precision e^x function -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/math/expf.h"
#include "common_constants.h" // Lookup tables EXP_M1 and EXP_M2.
#include "src/__support/FPUtil/BasicOperations.h"
#include "src/__support/FPUtil/FEnvImpl.h"
#include "src/__support/FPUtil/FMA.h"
#include "src/__support/FPUtil/FPBits.h"
#include "src/__support/FPUtil/PolyEval.h"
#include "src/__support/common.h"

#include <errno.h>

namespace __llvm_libc {

LLVM_LIBC_FUNCTION(float, expf, (float x)) {
  using FPBits = typename fputil::FPBits<float>;
  FPBits xbits(x);

  uint32_t x_u = xbits.uintval();
  uint32_t x_abs = x_u & 0x7fff'ffffU;

  // Exceptional values
  if (unlikely(x_u == 0xc236'bd8cU)) { // x = -0x1.6d7b18p+5f
    return 0x1.108a58p-66f - x * 0x1.0p-95f;
  }

  // When |x| >= 89, |x| < 2^-25, or x is nan
  if (unlikely(x_abs >= 0x42b2'0000U || x_abs <= 0x3280'0000U)) {
    // |x| < 2^-25
    if (xbits.get_unbiased_exponent() <= 101) {
      return 1.0f + x;
    }

    // When x < log(2^-150) or nan
    if (xbits.uintval() >= 0xc2cf'f1b5U) {
      // exp(-Inf) = 0
      if (xbits.is_inf())
        return 0.0f;
      // exp(nan) = nan
      if (xbits.is_nan())
        return x;
      if (fputil::get_round() == FE_UPWARD)
        return static_cast<float>(FPBits(FPBits::MIN_SUBNORMAL));
      errno = ERANGE;
      return 0.0f;
    }
    // x >= 89 or nan
    if (!xbits.get_sign() && (xbits.uintval() >= 0x42b2'0000)) {
      // x is finite
      if (xbits.uintval() < 0x7f80'0000U) {
        int rounding = fputil::get_round();
        if (rounding == FE_DOWNWARD || rounding == FE_TOWARDZERO)
          return static_cast<float>(FPBits(FPBits::MAX_NORMAL));

        errno = ERANGE;
      }
      // x is +inf or nan
      return x + static_cast<float>(FPBits::inf());
    }
  }
  // For -104 < x < 89, to compute exp(x), we perform the following range
  // reduction: find hi, mid, lo such that:
  //   x = hi + mid + lo, in which
  //     hi is an integer,
  //     mid * 2^7 is an integer
  //     -2^(-8) <= lo < 2^-8.
  // In particular,
  //   hi + mid = round(x * 2^7) * 2^(-7).
  // Then,
  //   exp(x) = exp(hi + mid + lo) = exp(hi) * exp(mid) * exp(lo).
  // We store exp(hi) and exp(mid) in the lookup tables EXP_M1 and EXP_M2
  // respectively.  exp(lo) is computed using a degree-4 minimax polynomial
  // generated by Sollya.

  // x_hi = (hi + mid) * 2^7 = round(x * 2^7).
  // The default rounding mode for float-to-int conversion in C++ is
  // round-toward-zero. To make it round-to-nearest, we add (-1)^sign(x) * 0.5
  // before conversion.
  int x_hi = static_cast<int>(x * 0x1.0p7f + (xbits.get_sign() ? -0.5f : 0.5f));
  // Subtract (hi + mid) from x to get lo.
  x -= static_cast<float>(x_hi) * 0x1.0p-7f;
  double xd = static_cast<double>(x);
  x_hi += 104 << 7;
  // hi = x_hi >> 7
  double exp_hi = EXP_M1[x_hi >> 7];
  // mid * 2^7 = x_hi & 0x0000'007fU;
  double exp_mid = EXP_M2[x_hi & 0x7f];
  // Degree-4 minimax polynomial generated by Sollya with the following
  // commands:
  //   > display = hexadecimal;
  //   > Q = fpminimax(expm1(x)/x, 3, [|D...|], [-2^-8, 2^-8]);
  //   > Q;
  double exp_lo =
      fputil::polyeval(xd, 0x1p0, 0x1.ffffffffff777p-1, 0x1.000000000071cp-1,
                       0x1.555566668e5e7p-3, 0x1.55555555ef243p-5);
  return static_cast<float>(exp_hi * exp_mid * exp_lo);
}

} // namespace __llvm_libc
