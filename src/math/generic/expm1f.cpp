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

  // When x < log(2^-25) or nan
  if (unlikely(xbits.uintval() >= 0xc18a'a123U)) {
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
  }
  // x >= 89 or nan
  if (unlikely(!xbits.get_sign() && (xbits.uintval() >= 0x42b2'0000))) {
    if (xbits.uintval() < 0x7f80'0000U) {
      int rounding = fputil::get_round();
      if (rounding == FE_DOWNWARD || rounding == FE_TOWARDZERO)
        return static_cast<float>(FPBits(FPBits::MAX_NORMAL));

      errno = ERANGE;
    }
    return x + static_cast<float>(FPBits::inf());
  }

  int unbiased_exponent = static_cast<int>(xbits.get_unbiased_exponent());
  // |x| < 2^-4
  if (unbiased_exponent < 123) {
    // |x| < 2^-25
    if (unbiased_exponent < 102) {
      // x = -0.0f
      if (unlikely(xbits.uintval() == 0x8000'0000U))
        return x;
      // When |x| < 2^-25, the relative error:
      //   |(e^x - 1) - x| / |x| < |x^2| / |x| = |x| < 2^-25 < epsilon(1)/2.
      // So the correctly rounded values of expm1(x) are:
      //   = x + eps(x) if rounding mode = FE_UPWARD,
      //                   or (rounding mode = FE_TOWARDZERO and x is negative),
      //   = x otherwise.
      // To simplify the rounding decision and make it more efficient, we use
      //   fma(x, x, x) ~ x + x^2 instead.
      return fputil::fma(x, x, x);
    }
    // 2^-25 <= |x| < 2^-4
    double xd = static_cast<double>(x);
    double xsq = xd * xd;
    // Degree-8 minimax polynomial generated by Sollya with:
    // > display = hexadecimal;
    // > P = fpminimax(expm1(x)/x, 7, [|D...|], [-2^-4, 2^-4]);
    double r =
        fputil::polyeval(xd, 0x1p-1, 0x1.55555555559abp-3, 0x1.55555555551a7p-5,
                         0x1.111110f70f2a4p-7, 0x1.6c16c17639e82p-10,
                         0x1.a02526febbea6p-13, 0x1.a01dc40888fcdp-16);
    return static_cast<float>(fputil::fma(r, xsq, xd));
  }

  // For -18 < x < 89, to compute exp(x), we perform the following range
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
  // respectively.  exp(lo) is computed using a degree-7 minimax polynomial
  // generated by Sollya.

  // Exceptional value
  if (xbits.uintval() == 0xbdc1'c6cbU) {
    // x = -0x1.838d96p-4f
    int round_mode = fputil::get_round();
    if (round_mode == FE_TONEAREST || round_mode == FE_DOWNWARD)
      return -0x1.71c884p-4f;
    return -0x1.71c882p-4f;
  }

  // x_hi = hi + mid.
  int x_hi = static_cast<int>(x * 0x1.0p7f);
  // Subtract (hi + mid) from x to get lo.
  x -= static_cast<float>(x_hi) * 0x1.0p-7f;
  double xd = static_cast<double>(x);
  // Make sure that -2^(-8) <= lo < 2^-8.
  if (x >= 0x1.0p-8f) {
    ++x_hi;
    xd -= 0x1.0p-7;
  }
  if (x < -0x1.0p-8f) {
    --x_hi;
    xd += 0x1.0p-7;
  }
  x_hi += 104 << 7;
  // hi = x_hi >> 7
  double exp_hi = EXP_M1[x_hi >> 7];
  // lo = x_hi & 0x0000'007fU;
  double exp_mid = EXP_M2[x_hi & 0x7f];
  double exp_hi_mid = exp_hi * exp_mid;
  // Degree-7 minimax polynomial generated by Sollya with the following
  // commands:
  //   > display = hexadecimal;
  //   > Q = fpminimax(expm1(x)/x, 6, [|D...|], [-2^-8, 2^-8]);
  //   > Q;
  double exp_lo = fputil::polyeval(
      xd, 0x1p0, 0x1p0, 0x1p-1, 0x1.5555555555555p-3, 0x1.55555555553ap-5,
      0x1.1111111204dfcp-7, 0x1.6c16cb2da593ap-10, 0x1.9ff1648996d2ep-13);
  return static_cast<float>(fputil::fma(exp_hi_mid, exp_lo, -1.0));
}

} // namespace __llvm_libc
