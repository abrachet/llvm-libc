//===-- Unittests for strtol ---------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/stdlib/strtol.h"

#include "utils/UnitTest/Test.h"

#include <errno.h>
#include <limits.h>

TEST(LlvmLibcStrToLTest, InvalidBase) {
  const char *ten = "10";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(ten, nullptr, -1), 0l);
  ASSERT_EQ(errno, EINVAL);
}

TEST(LlvmLibcStrToLTest, CleanBaseTenDecode) {
  char *str_end = nullptr;

  const char *ten = "10";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(ten, &str_end, 10), 10l);
  ASSERT_EQ(errno, 0);
  EXPECT_EQ(str_end - ten, 2l);
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(ten, nullptr, 10), 10l);
  ASSERT_EQ(errno, 0);

  const char *hundred = "100";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(hundred, &str_end, 10), 100l);
  ASSERT_EQ(errno, 0);
  EXPECT_EQ(str_end - hundred, 3l);

  const char *negative = "-100";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(negative, &str_end, 10), -100l);
  ASSERT_EQ(errno, 0);
  EXPECT_EQ(str_end - negative, 4l);

  const char *big_number = "123456789012345";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(big_number, &str_end, 10), 123456789012345l);
  ASSERT_EQ(errno, 0);
  EXPECT_EQ(str_end - big_number, 15l);

  const char *big_negative_number = "-123456789012345";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(big_negative_number, &str_end, 10),
            -123456789012345l);
  ASSERT_EQ(errno, 0);
  EXPECT_EQ(str_end - big_negative_number, 16l);

  const char *too_big_number = "123456789012345678901";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(too_big_number, &str_end, 10), LONG_MAX);
  ASSERT_EQ(errno, ERANGE);
  EXPECT_EQ(str_end - too_big_number, 21l);

  const char *too_big_negative_number = "-123456789012345678901";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(too_big_negative_number, &str_end, 10),
            LONG_MIN);
  ASSERT_EQ(errno, ERANGE);
  EXPECT_EQ(str_end - too_big_negative_number, 22l);

  const char *long_number_range_test =
      "10000000000000000000000000000000000000000000000000";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(long_number_range_test, &str_end, 10),
            LONG_MAX);
  ASSERT_EQ(errno, ERANGE);
  EXPECT_EQ(str_end - long_number_range_test, 50l);
}

TEST(LlvmLibcStrToLTest, MessyBaseTenDecode) {
  char *str_end = nullptr;

  const char *spaces_before = "     10";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(spaces_before, &str_end, 10), 10l);
  ASSERT_EQ(errno, 0);
  EXPECT_EQ(str_end - spaces_before, 7l);

  const char *spaces_after = "10      ";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(spaces_after, &str_end, 10), 10l);
  ASSERT_EQ(errno, 0);
  EXPECT_EQ(str_end - spaces_after, 2l);

  const char *word_before = "word10";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(word_before, &str_end, 10), 0l);
  ASSERT_EQ(errno, 0);
  EXPECT_EQ(str_end - word_before, 0l);

  const char *word_after = "10word";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(word_after, &str_end, 10), 10l);
  ASSERT_EQ(errno, 0);
  EXPECT_EQ(str_end - word_after, 2l);

  const char *two_numbers = "10 999";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(two_numbers, &str_end, 10), 10l);
  ASSERT_EQ(errno, 0);
  EXPECT_EQ(str_end - two_numbers, 2l);

  const char *two_signs = "--10 999";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(two_signs, &str_end, 10), 0l);
  ASSERT_EQ(errno, 0);
  EXPECT_EQ(str_end - two_signs, 1l);

  const char *sign_before = "+2=4";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(sign_before, &str_end, 10), 2l);
  ASSERT_EQ(errno, 0);
  EXPECT_EQ(str_end - sign_before, 2l);

  const char *sign_after = "2+2=4";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(sign_after, &str_end, 10), 2l);
  ASSERT_EQ(errno, 0);
  EXPECT_EQ(str_end - sign_after, 1l);

  const char *tab_before = "\t10";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(tab_before, &str_end, 10), 10l);
  ASSERT_EQ(errno, 0);
  EXPECT_EQ(str_end - tab_before, 3l);

  const char *all_together = "\t  -12345and+67890";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(all_together, &str_end, 10), -12345l);
  ASSERT_EQ(errno, 0);
  EXPECT_EQ(str_end - all_together, 9l);
}

static char int_to_b36_char(int input) {
  if (input < 0 || input > 36)
    return '0';
  if (input < 10)
    return '0' + input;
  return 'A' + input - 10;
}

TEST(LlvmLibcStrToLTest, DecodeInOtherBases) {
  char small_string[4] = {'\0', '\0', '\0', '\0'};
  for (unsigned int base = 2; base <= 36; ++base) {
    for (long first_digit = 0; first_digit <= 36; ++first_digit) {
      small_string[0] = int_to_b36_char(first_digit);
      if (first_digit < base) {
        errno = 0;
        ASSERT_EQ(__llvm_libc::strtol(small_string, nullptr, base),
                  first_digit);
        ASSERT_EQ(errno, 0);
      } else {
        errno = 0;
        ASSERT_EQ(__llvm_libc::strtol(small_string, nullptr, base), 0l);
        ASSERT_EQ(errno, 0);
      }
    }
  }

  for (unsigned int base = 2; base <= 36; ++base) {
    for (long first_digit = 0; first_digit <= 36; ++first_digit) {
      small_string[0] = int_to_b36_char(first_digit);
      for (long second_digit = 0; second_digit <= 36; ++second_digit) {
        small_string[1] = int_to_b36_char(second_digit);
        if (first_digit < base && second_digit < base) {
          errno = 0;
          ASSERT_EQ(__llvm_libc::strtol(small_string, nullptr, base),
                    second_digit + (first_digit * base));
          ASSERT_EQ(errno, 0);
        } else if (first_digit < base) {
          errno = 0;
          ASSERT_EQ(__llvm_libc::strtol(small_string, nullptr, base),
                    first_digit);
          ASSERT_EQ(errno, 0);
        } else {
          errno = 0;
          ASSERT_EQ(__llvm_libc::strtol(small_string, nullptr, base), 0l);
          ASSERT_EQ(errno, 0);
        }
      }
    }
  }

  for (unsigned int base = 2; base <= 36; ++base) {
    for (long first_digit = 0; first_digit <= 36; ++first_digit) {
      small_string[0] = int_to_b36_char(first_digit);
      for (long second_digit = 0; second_digit <= 36; ++second_digit) {
        small_string[1] = int_to_b36_char(second_digit);
        for (long third_digit = 0; third_digit <= 36; ++third_digit) {
          small_string[2] = int_to_b36_char(third_digit);

          if (first_digit < base && second_digit < base && third_digit < base) {
            errno = 0;
            ASSERT_EQ(__llvm_libc::strtol(small_string, nullptr, base),
                      third_digit + (second_digit * base) +
                          (first_digit * base * base));
            ASSERT_EQ(errno, 0);
          } else if (first_digit < base && second_digit < base) {
            errno = 0;
            ASSERT_EQ(__llvm_libc::strtol(small_string, nullptr, base),
                      second_digit + (first_digit * base));
            ASSERT_EQ(errno, 0);
          } else if (first_digit < base) {
            // if the base is 16 there is a special case for the prefix 0X.
            // The number is treated as a one digit hexadecimal.
            if (base == 16 && first_digit == 0 && second_digit == 33) {
              if (third_digit < base) {
                errno = 0;
                ASSERT_EQ(__llvm_libc::strtol(small_string, nullptr, base),
                          third_digit);
                ASSERT_EQ(errno, 0);
              } else {
                errno = 0;
                ASSERT_EQ(__llvm_libc::strtol(small_string, nullptr, base), 0l);
                ASSERT_EQ(errno, 0);
              }
            } else {
              errno = 0;
              ASSERT_EQ(__llvm_libc::strtol(small_string, nullptr, base),
                        first_digit);
              ASSERT_EQ(errno, 0);
            }
          } else {
            errno = 0;
            ASSERT_EQ(__llvm_libc::strtol(small_string, nullptr, base), 0l);
            ASSERT_EQ(errno, 0);
          }
        }
      }
    }
  }
}

TEST(LlvmLibcStrToLTest, CleanBaseSixteenDecode) {
  char *str_end = nullptr;

  const char *no_prefix = "123abc";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(no_prefix, &str_end, 16), 0x123abcl);
  ASSERT_EQ(errno, 0);
  EXPECT_EQ(str_end - no_prefix, 6l);

  const char *yes_prefix = "0x456def";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(yes_prefix, &str_end, 16), 0x456defl);
  ASSERT_EQ(errno, 0);
  EXPECT_EQ(str_end - yes_prefix, 8l);
}

TEST(LlvmLibcStrToLTest, AutomaticBaseSelection) {
  char *str_end = nullptr;

  const char *base_ten = "12345";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(base_ten, &str_end, 0), 12345l);
  ASSERT_EQ(errno, 0);
  EXPECT_EQ(str_end - base_ten, 5l);

  const char *base_sixteen_no_prefix = "123abc";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(base_sixteen_no_prefix, &str_end, 0), 123l);
  ASSERT_EQ(errno, 0);
  EXPECT_EQ(str_end - base_sixteen_no_prefix, 3l);

  const char *base_sixteen_with_prefix = "0x456def";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(base_sixteen_with_prefix, &str_end, 0),
            0x456defl);
  ASSERT_EQ(errno, 0);
  EXPECT_EQ(str_end - base_sixteen_with_prefix, 8l);

  const char *base_eight_with_prefix = "012345";
  errno = 0;
  ASSERT_EQ(__llvm_libc::strtol(base_eight_with_prefix, &str_end, 0), 012345l);
  ASSERT_EQ(errno, 0);
  EXPECT_EQ(str_end - base_eight_with_prefix, 6l);
}
