//===-- Single-precision e^x - 1 function ---------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/math/expm1f.h"
#include "common_constants.h" // Lookup tables EXP_M1 and EXP_M2.
#include "src/__support/FPUtil/BasicOperations.h"
#include "src/__support/FPUtil/FEnvImpl.h"
#include "src/__support/FPUtil/FMA.h"
#include "src/__support/FPUtil/FPBits.h"
#include "src/__support/FPUtil/PolyEval.h"
#include "src/__support/common.h"

#include <errno.h>

namespace __llvm_libc {

INLINE_FMA
LLVM_LIBC_FUNCTION(float, expm1f, (float x)) {
  using FPBits = typename fputil::FPBits<float>;
  FPBits xbits(x);

  uint32_t x_u = xbits.uintval();
  uint32_t x_abs = x_u & 0x7fff'ffffU;

  // Exceptional value
  if (unlikely(x_u == 0x3e35'bec5U)) { // x = 0x1.6b7d8ap-3f
    int round_mode = fputil::get_round();
    if (round_mode == FE_TONEAREST || round_mode == FE_UPWARD)
      return 0x1.8dbe64p-3f;
    return 0x1.8dbe62p-3f;
  }

  // When |x| > 25*log(2), or nan
  if (unlikely(x_abs >= 0x418a'a123U)) {
    // x < log(2^-25)
    if (xbits.get_sign()) {
      // exp(-Inf) = 0
      if (xbits.is_inf())
        return -1.0f;
      // exp(nan) = nan
      if (xbits.is_nan())
        return x;
      int round_mode = fputil::get_round();
      if (round_mode == FE_UPWARD || round_mode == FE_TOWARDZERO)
        return -0x1.ffff'fep-1f; // -1.0f + 0x1.0p-24f
      return -1.0f;
    } else {
      // x >= 89 or nan
      if (xbits.uintval() >= 0x42b2'0000) {
        if (xbits.uintval() < 0x7f80'0000U) {
          int rounding = fputil::get_round();
          if (rounding == FE_DOWNWARD || rounding == FE_TOWARDZERO)
            return static_cast<float>(FPBits(FPBits::MAX_NORMAL));

          errno = ERANGE;
        }
        return x + static_cast<float>(FPBits::inf());
      }
    }
  }

  // |x| < 2^-4
  if (x_abs < 0x3d80'0000U) {
    // |x| < 2^-25
    if (x_abs < 0x3300'0000U) {
      // x = -0.0f
      if (unlikely(xbits.uintval() == 0x8000'0000U))
        return x;
      // When |x| < 2^-25, the relative error of the approximation e^x - 1 ~ x
      // is:
      //   |(e^x - 1) - x| / |e^x - 1| < |x^2| / |x|
      //                               = |x|
      //                               < 2^-25
      //                               < epsilon(1)/2.
      // So the correctly rounded values of expm1(x) are:
      //   = x + eps(x) if rounding mode = FE_UPWARD,
      //                   or (rounding mode = FE_TOWARDZERO and x is negative),
      //   = x otherwise.
      // To simplify the rounding decision and make it more efficient, we use
      //   fma(x, x, x) ~ x + x^2 instead.
      return fputil::multiply_add(x, x, x);
    }

    // 2^-25 <= |x| < 2^-4
    double xd = static_cast<double>(x);
    double xsq = xd * xd;
    // Degree-8 minimax polynomial generated by Sollya with:
    // > display = hexadecimal;
    // > P = fpminimax((expm1(x) - x)/x^2, 6, [|D...|], [-2^-4, 2^-4]);
    double r =
        fputil::polyeval(xd, 0x1p-1, 0x1.55555555557ddp-3, 0x1.55555555552fap-5,
                         0x1.111110fcd58b7p-7, 0x1.6c16c1717660bp-10,
                         0x1.a0241f0006d62p-13, 0x1.a01e3f8d3c06p-16);
    return static_cast<float>(fputil::multiply_add(r, xsq, xd));
  }

  // For -18 < x < 89, to compute expm1(x), we perform the following range
  // reduction: find hi, mid, lo such that:
  //   x = hi + mid + lo, in which
  //     hi is an integer,
  //     mid * 2^7 is an integer
  //     -2^(-8) <= lo < 2^-8.
  // In particular,
  //   hi + mid = round(x * 2^7) * 2^(-7).
  // Then,
  //   expm1(x) = exp(hi + mid + lo) - 1 = exp(hi) * exp(mid) * exp(lo) - 1.
  // We store exp(hi) and exp(mid) in the lookup tables EXP_M1 and EXP_M2
  // respectively.  exp(lo) is computed using a degree-4 minimax polynomial
  // generated by Sollya.

  // x_hi = hi + mid.
  int x_hi = static_cast<int>(x * 0x1.0p7f + (xbits.get_sign() ? -0.5f : 0.5f));
  // Subtract (hi + mid) from x to get lo.
  x -= static_cast<float>(x_hi) * 0x1.0p-7f;
  double xd = static_cast<double>(x);
  x_hi += 104 << 7;
  // hi = x_hi >> 7
  double exp_hi = EXP_M1[x_hi >> 7];
  // lo = x_hi & 0x0000'007fU;
  double exp_mid = EXP_M2[x_hi & 0x7f];
  double exp_hi_mid = exp_hi * exp_mid;
  // Degree-4 minimax polynomial generated by Sollya with the following
  // commands:
  //   > display = hexadecimal;
  //   > Q = fpminimax(expm1(x)/x, 3, [|D...|], [-2^-8, 2^-8]);
  //   > Q;
  double exp_lo =
      fputil::polyeval(xd, 0x1.0p0, 0x1.ffffffffff777p-1, 0x1.000000000071cp-1,
                       0x1.555566668e5e7p-3, 0x1.55555555ef243p-5);
  return static_cast<float>(fputil::multiply_add(exp_hi_mid, exp_lo, -1.0));
}

} // namespace __llvm_libc
