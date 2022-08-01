//===-- Single-precision sincos function ----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/math/sincosf.h"
#include "sincosf_utils.h"
#include "src/__support/FPUtil/FEnvImpl.h"
#include "src/__support/FPUtil/FPBits.h"
#include "src/__support/FPUtil/multiply_add.h"
#include "src/__support/common.h"

#include <errno.h>

namespace __llvm_libc {

// Exceptional values
static constexpr int N_EXCEPTS = 10;

static constexpr uint32_t EXCEPT_INPUTS[N_EXCEPTS] = {
    0x3b5637f5, // x = 0x1.ac6feap-9
    0x3fa7832a, // x = 0x1.4f0654p0
    0x46199998, // x = 0x1.33333p13
    0x55325019, // x = 0x1.64a032p43
    0x55cafb2a, // x = 0x1.95f654p44
    0x5922aa80, // x = 0x1.4555p51
    0x5aa4542c, // x = 0x1.48a858p54
    0x5f18b878, // x = 0x1.3170fp63
    0x6115cb11, // x = 0x1.2b9622p67
    0x7beef5ef, // x = 0x1.ddebdep120
};

static constexpr uint32_t EXCEPT_OUTPUTS_SIN[N_EXCEPTS][4] = {
    {0x3b5637dc, 1, 0, 0}, // x = 0x1.ac6feap-9, sin(x) = 0x1.ac6fb8p-9 (RZ)
    {0x3f7741b5, 1, 0, 1}, // x = 0x1.4f0654p0, sin(x) = 0x1.ee836ap-1 (RZ)
    {0xbeb1fa5d, 0, 1, 0}, // x = 0x1.33333p13, sin(x) = -0x1.63f4bap-2 (RZ)
    {0xbf171adf, 0, 1, 1}, // x = 0x1.64a032p43, sin(x) = -0x1.2e35bep-1 (RZ)
    {0xbf7e7a16, 0, 1, 1}, // x = 0x1.95f654p44, sin(x) = -0x1.fcf42cp-1 (RZ)
    {0xbf587521, 0, 1, 1}, // x = 0x1.4555p51, sin(x) = -0x1.b0ea42p-1 (RZ)
    {0x3f5f5646, 1, 0, 0}, // x = 0x1.48a858p54, sin(x) = 0x1.beac8cp-1 (RZ)
    {0x3dad60f6, 1, 0, 1}, // x = 0x1.3170fp63, sin(x) = 0x1.5ac1ecp-4 (RZ)
    {0xbe7cc1e0, 0, 1, 1}, // x = 0x1.2b9622p67, sin(x) = -0x1.f983cp-3 (RZ)
    {0xbf587d1b, 0, 1, 1}, // x = 0x1.ddebdep120, sin(x) = -0x1.b0fa36p-1 (RZ)
};

static constexpr uint32_t EXCEPT_OUTPUTS_COS[N_EXCEPTS][4] = {
    {0x3f7fffa6, 1, 0, 0}, // x = 0x1.ac6feap-9, cos(x) = 0x1.ffff4cp-1 (RZ)
    {0x3e84aabf, 1, 0, 1}, // x = 0x1.4f0654p0, cos(x) = 0x1.09557ep-2 (RZ)
    {0xbf70090b, 0, 1, 0}, // x = 0x1.33333p13, cos(x) = -0x1.e01216p-1 (RZ)
    {0x3f4ea5d2, 1, 0, 0}, // x = 0x1.64a032p43, cos(x) = 0x1.9d4ba4p-1 (RZ)
    {0x3ddf11f3, 1, 0, 1}, // x = 0x1.95f654p44, cos(x) = 0x1.be23e6p-4 (RZ)
    {0x3f08aebe, 1, 0, 1}, // x = 0x1.4555p51, cos(x) = 0x1.115d7cp-1 (RZ)
    {0x3efa40a4, 1, 0, 0}, // x = 0x1.48a858p54, cos(x) = 0x1.f48148p-2 (RZ)
    {0x3f7f14bb, 1, 0, 0}, // x = 0x1.3170fp63, cos(x) = 0x1.fe2976p-1 (RZ)
    {0x3f78142e, 1, 0, 1}, // x = 0x1.2b9622p67, cos(x) = 0x1.f0285cp-1 (RZ)
    {0x3f08a21c, 1, 0, 0}, // x = 0x1.ddebdep120, cos(x) = 0x1.114438p-1 (RZ)
};

// Fast sincosf implementation. Worst-case ULP is 0.5607, maximum relative
// error is 0.5303 * 2^-23. A single-step range reduction is used for
// small values. Large inputs have their range reduced using fast integer
// arithmetic.
LLVM_LIBC_FUNCTION(void, sincosf, (float x, float *sinp, float *cosp)) {
  using FPBits = typename fputil::FPBits<float>;
  FPBits xbits(x);

  uint32_t x_abs = xbits.uintval() & 0x7fff'ffffU;
  double xd = static_cast<double>(x);

  // Range reduction:
  // For |x| > pi/16, we perform range reduction as follows:
  // Find k and y such that:
  //   x = (k + y) * pi/16
  //   k is an integer
  //   |y| < 0.5
  // For small range (|x| < 2^46 when FMA instructions are available, 2^22
  // otherwise), this is done by performing:
  //   k = round(x * 16/pi)
  //   y = x * 16/pi - k
  // For large range, we will omit all the higher parts of 16/pi such that the
  // least significant bits of their full products with x are larger than 31,
  // since:
  //     sin((k + y + 32*i) * pi/16) = sin(x + i * 2pi) = sin(x), and
  //     cos((k + y + 32*i) * pi/16) = cos(x + i * 2pi) = cos(x).
  //
  // When FMA instructions are not available, we store the digits of 16/pi in
  // chunks of 28-bit precision.  This will make sure that the products:
  //   x * SIXTEEN_OVER_PI_28[i] are all exact.
  // When FMA instructions are available, we simply store the digits of 16/pi in
  // chunks of doubles (53-bit of precision).
  // So when multiplying by the largest values of single precision, the
  // resulting output should be correct up to 2^(-208 + 128) ~ 2^-80.  By the
  // worst-case analysis of range reduction, |y| >= 2^-38, so this should give
  // us more than 40 bits of accuracy. For the worst-case estimation of range
  // reduction, see for instances:
  //   Elementary Functions by J-M. Muller, Chapter 11,
  //   Handbook of Floating-Point Arithmetic by J-M. Muller et. al.,
  //   Chapter 10.2.
  //
  // Once k and y are computed, we then deduce the answer by the sine and cosine
  // of sum formulas:
  //   sin(x) = sin((k + y)*pi/16)
  //          = sin(y*pi/16) * cos(k*pi/16) + cos(y*pi/16) * sin(k*pi/16)
  //   cos(x) = cos((k + y)*pi/16)
  //          = cos(y*pi/16) * cos(k*pi/16) - sin(y*pi/16) * sin(k*pi/16)
  // The values of sin(k*pi/16) and cos(k*pi/16) for k = 0..31 are precomputed
  // and stored using a vector of 32 doubles. Sin(y*pi/16) and cos(y*pi/16) are
  // computed using degree-7 and degree-8 minimax polynomials generated by
  // Sollya respectively.

  // |x| < 0x1.0p-12f
  if (unlikely(x_abs < 0x3980'0000U)) {
    if (unlikely(x_abs == 0U)) {
      // For signed zeros.
      *sinp = x;
      *cosp = 1.0f;
      return;
    }
    // When |x| < 2^-12, the relative errors of the approximations
    //   sin(x) ~ x, cos(x) ~ 1
    // are:
    //   |sin(x) - x| / |sin(x)| < |x^3| / (6|x|)
    //                           = x^2 / 6
    //                           < 2^-25
    //                           < epsilon(1)/2.
    //   |cos(x) - 1| < |x^2 / 2| = 2^-25 < epsilon(1)/2.
    // So the correctly rounded values of sin(x) and cos(x) are:
    //   sin(x) = x - sign(x)*eps(x) if rounding mode = FE_TOWARDZERO,
    //                        or (rounding mode = FE_UPWARD and x is
    //                        negative),
    //          = x otherwise.
    //   cos(x) = 1 - eps(x) if rounding mode = FE_TOWARDZERO or FE_DOWWARD,
    //          = 1 otherwise.
    // To simplify the rounding decision and make it more efficient and to
    // prevent compiler to perform constant folding, we use
    //   sin(x) = fma(x, -2^-25, x),
    //   cos(x) = fma(x*0.5f, -x, 1)
    // instead.
    // Note: to use the formula x - 2^-25*x to decide the correct rounding, we
    // do need fma(x, -2^-25, x) to prevent underflow caused by -2^-25*x when
    // |x| < 2^-125. For targets without FMA instructions, we simply use
    // double for intermediate results as it is more efficient than using an
    // emulated version of FMA.
#if defined(LIBC_TARGET_HAS_FMA)
    *sinp = fputil::multiply_add(x, -0x1.0p-25f, x);
    *cosp = fputil::multiply_add(FPBits(x_abs).get_val(), -0x1.0p-25f, 1.0f);
#else
    *sinp = static_cast<float>(fputil::multiply_add(xd, -0x1.0p-25, xd));
    *cosp = static_cast<float>(fputil::multiply_add(
        static_cast<double>(FPBits(x_abs).get_val()), -0x1.0p-25, 1.0));
#endif // LIBC_TARGET_HAS_FMA
    return;
  }

  // x is inf or nan.
  if (unlikely(x_abs >= 0x7f80'0000U)) {
    if (x_abs == 0x7f80'0000U) {
      errno = EDOM;
      fputil::set_except(FE_INVALID);
    }
    *sinp =
        x + FPBits::build_nan(1 << (fputil::MantissaWidth<float>::VALUE - 1));
    *cosp = *sinp;
    return;
  }

  // Check exceptional values.
  for (int i = 0; i < N_EXCEPTS; ++i) {
    if (unlikely(x_abs == EXCEPT_INPUTS[i])) {
      uint32_t s = EXCEPT_OUTPUTS_SIN[i][0]; // FE_TOWARDZERO
      uint32_t c = EXCEPT_OUTPUTS_COS[i][0]; // FE_TOWARDZERO
      bool x_sign = x < 0;
      switch (fputil::get_round()) {
      case FE_UPWARD:
        s += x_sign ? EXCEPT_OUTPUTS_SIN[i][2] : EXCEPT_OUTPUTS_SIN[i][1];
        c += EXCEPT_OUTPUTS_COS[i][1];
        break;
      case FE_DOWNWARD:
        s += x_sign ? EXCEPT_OUTPUTS_SIN[i][1] : EXCEPT_OUTPUTS_SIN[i][2];
        c += EXCEPT_OUTPUTS_COS[i][2];
        break;
      case FE_TONEAREST:
        s += EXCEPT_OUTPUTS_SIN[i][3];
        c += EXCEPT_OUTPUTS_COS[i][3];
        break;
      }
      *sinp = x_sign ? -FPBits(s).get_val() : FPBits(s).get_val();
      *cosp = FPBits(c).get_val();

      return;
    }
  }

  // Combine the results with the sine and cosine of sum formulas:
  //   sin(x) = sin((k + y)*pi/16)
  //          = sin(y*pi/16) * cos(k*pi/16) + cos(y*pi/16) * sin(k*pi/16)
  //          = sin_y * cos_k + (1 + cosm1_y) * sin_k
  //          = sin_y * cos_k + (cosm1_y * sin_k + sin_k)
  //   cos(x) = cos((k + y)*pi/16)
  //          = cos(y*pi/16) * cos(k*pi/16) - sin(y*pi/16) * sin(k*pi/16)
  //          = cosm1_y * cos_k + sin_y * sin_k
  //          = (cosm1_y * cos_k + cos_k) + sin_y * sin_k
  double sin_k, cos_k, sin_y, cosm1_y;

  sincosf_eval(xd, x_abs, sin_k, cos_k, sin_y, cosm1_y);

  *sinp = fputil::multiply_add(sin_y, cos_k,
                               fputil::multiply_add(cosm1_y, sin_k, sin_k));
  *cosp = fputil::multiply_add(sin_y, -sin_k,
                               fputil::multiply_add(cosm1_y, cos_k, cos_k));
}

} // namespace __llvm_libc
