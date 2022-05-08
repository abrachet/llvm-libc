//===-- Single-precision 2^x function -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/math/exp2f.h"
#include "src/__support/FPUtil/BasicOperations.h"
#include "src/__support/FPUtil/FEnvImpl.h"
#include "src/__support/FPUtil/FMA.h"
#include "src/__support/FPUtil/FPBits.h"
#include "src/__support/FPUtil/PolyEval.h"
#include "src/__support/common.h"

#include <errno.h>

namespace __llvm_libc {

// Lookup table for 2^(m * 2^(-6)) with m = 0, ..., 63.
// Table is generated with Sollya as follow:
// > display = hexadecimal;
// > for i from 0 to 63 do { D(2^(i / 64)); };
static constexpr double EXP_M[64] = {
    0x1.0000000000000p0, 0x1.02c9a3e778061p0, 0x1.059b0d3158574p0,
    0x1.0874518759bc8p0, 0x1.0b5586cf9890fp0, 0x1.0e3ec32d3d1a2p0,
    0x1.11301d0125b51p0, 0x1.1429aaea92de0p0, 0x1.172b83c7d517bp0,
    0x1.1a35beb6fcb75p0, 0x1.1d4873168b9aap0, 0x1.2063b88628cd6p0,
    0x1.2387a6e756238p0, 0x1.26b4565e27cddp0, 0x1.29e9df51fdee1p0,
    0x1.2d285a6e4030bp0, 0x1.306fe0a31b715p0, 0x1.33c08b26416ffp0,
    0x1.371a7373aa9cbp0, 0x1.3a7db34e59ff7p0, 0x1.3dea64c123422p0,
    0x1.4160a21f72e2ap0, 0x1.44e086061892dp0, 0x1.486a2b5c13cd0p0,
    0x1.4bfdad5362a27p0, 0x1.4f9b2769d2ca7p0, 0x1.5342b569d4f82p0,
    0x1.56f4736b527dap0, 0x1.5ab07dd485429p0, 0x1.5e76f15ad2148p0,
    0x1.6247eb03a5585p0, 0x1.6623882552225p0, 0x1.6a09e667f3bcdp0,
    0x1.6dfb23c651a2fp0, 0x1.71f75e8ec5f74p0, 0x1.75feb564267c9p0,
    0x1.7a11473eb0187p0, 0x1.7e2f336cf4e62p0, 0x1.82589994cce13p0,
    0x1.868d99b4492edp0, 0x1.8ace5422aa0dbp0, 0x1.8f1ae99157736p0,
    0x1.93737b0cdc5e5p0, 0x1.97d829fde4e50p0, 0x1.9c49182a3f090p0,
    0x1.a0c667b5de565p0, 0x1.a5503b23e255dp0, 0x1.a9e6b5579fdbfp0,
    0x1.ae89f995ad3adp0, 0x1.b33a2b84f15fbp0, 0x1.b7f76f2fb5e47p0,
    0x1.bcc1e904bc1d2p0, 0x1.c199bdd85529cp0, 0x1.c67f12e57d14bp0,
    0x1.cb720dcef9069p0, 0x1.d072d4a07897cp0, 0x1.d5818dcfba487p0,
    0x1.da9e603db3285p0, 0x1.dfc97337b9b5fp0, 0x1.e502ee78b3ff6p0,
    0x1.ea4afa2a490dap0, 0x1.efa1bee615a27p0, 0x1.f50765b6e4540p0,
    0x1.fa7c1819e90d8p0,
};

LLVM_LIBC_FUNCTION(float, exp2f, (float x)) {
  using FPBits = typename fputil::FPBits<float>;
  FPBits xbits(x);

  uint32_t x_u = xbits.uintval();
  uint32_t x_abs = x_u & 0x7fff'ffffU;

  // Exceptional values.
  switch (x_u) {
  case 0x3b42'9d37U: // x = 0x1.853a6ep-9f
    if (fputil::get_round() == FE_TONEAREST)
      return 0x1.00870ap+0f;
    break;
  case 0x3c02'a9adU: // x = 0x1.05535ap-7f
    if (fputil::get_round() == FE_TONEAREST)
      return 0x1.016b46p+0f;
    break;
  case 0x3ca6'6e26U: { // x = 0x1.4cdc4cp-6f
    int round_mode = fputil::get_round();
    if (round_mode == FE_TONEAREST || round_mode == FE_UPWARD)
      return 0x1.03a16ap+0f;
    return 0x1.03a168p+0f;
  }
  case 0x3d92'a282U: // x = 0x1.254504p-4f
    if (fputil::get_round() == FE_UPWARD)
      return 0x1.0d0688p+0f;
    return 0x1.0d0686p+0f;
  case 0xbcf3'a937U: // x = -0x1.e7526ep-6f
    if (fputil::get_round() == FE_TONEAREST)
      return 0x1.f58d62p-1f;
    break;
  case 0xb8d3'd026U: // x = -0x1.a7a04cp-14f
    if (fputil::get_round() == FE_TONEAREST)
      return 0x1.fff6d2p-1f;
    break;
  }

  // // When |x| >= 128, |x| < 2^-25, or x is nan
  if (unlikely(x_abs >= 0x4300'0000U || x_abs <= 0x3280'0000U)) {
    // |x| < 2^-25
    if (x_abs <= 0x3280'0000U) {
      return 1.0f + x;
    }
    // x >= 128
    if (!xbits.get_sign()) {
      // x is finite
      if (x_u < 0x7f80'0000U) {
        int rounding = fputil::get_round();
        if (rounding == FE_DOWNWARD || rounding == FE_TOWARDZERO)
          return static_cast<float>(FPBits(FPBits::MAX_NORMAL));

        errno = ERANGE;
      }
      // x is +inf or nan
      return x + static_cast<float>(FPBits::inf());
    }
    // x < -150
    if (x_u >= 0xc316'0000U) {
      // exp(-Inf) = 0
      if (xbits.is_inf())
        return 0.0f;
      // exp(nan) = nan
      if (xbits.is_nan())
        return x;
      if (fputil::get_round() == FE_UPWARD)
        return static_cast<float>(FPBits(FPBits::MIN_SUBNORMAL));
      if (x != 0.0f)
        errno = ERANGE;
      return 0.0f;
    }
  }
  // For -150 <= x < 128, to compute 2^x, we perform the following range
  // reduction: find hi, mid, lo such that:
  //   x = hi + mid + lo, in which
  //     hi is an integer,
  //     mid * 2^6 is an integer
  //     -2^(-7) <= lo < 2^-7.
  // In particular,
  //   hi + mid = round(x * 2^6) * 2^(-6).
  // Then,
  //   2^(x) = 2^(hi + mid + lo) = 2^hi * 2^mid * 2^lo.
  // Multiply by 2^hi is simply adding hi to the exponent field.  We store
  // exp(mid) in the lookup tables EXP_M.  exp(lo) is computed using a degree-4
  // minimax polynomial generated by Sollya.

  // x_hi = round(hi + mid).
  // The default rounding mode for float-to-int conversion in C++ is
  // round-toward-zero. To make it round-to-nearest, we add (-1)^sign(x) * 0.5
  // before conversion.
  int x_hi =
      static_cast<int>(x * 0x1.0p+6f + (xbits.get_sign() ? -0.5f : 0.5f));
  // For 2-complement integers, arithmetic right shift is the same as dividing
  // by a power of 2 and then round down (toward negative infinity).
  int e_hi = (x_hi >> 6) +
             static_cast<int>(fputil::FloatProperties<double>::EXPONENT_BIAS);
  fputil::FPBits<double> exp_hi(
      static_cast<uint64_t>(e_hi)
      << fputil::FloatProperties<double>::MANTISSA_WIDTH);
  // mid = x_hi & 0x0000'003fU;
  double exp_hi_mid = static_cast<double>(exp_hi) * EXP_M[x_hi & 0x3f];
  // Subtract (hi + mid) from x to get lo.
  x -= static_cast<float>(x_hi) * 0x1.0p-6f;
  double xd = static_cast<double>(x);
  // Degree-4 minimax polynomial generated by Sollya with the following
  // commands:
  //   > display = hexadecimal;
  //   > Q = fpminimax((2^x - 1)/x, 3, [|D...|], [-2^-7, 2^-7]);
  //   > Q;
  double exp_lo =
      fputil::polyeval(xd, 0x1p0, 0x1.62e42fefa2417p-1, 0x1.ebfbdff82f809p-3,
                       0x1.c6b0b92131c47p-5, 0x1.3b2ab6fb568a3p-7);
  double result = exp_hi_mid * exp_lo;
  return static_cast<float>(result);
}

} // namespace __llvm_libc
