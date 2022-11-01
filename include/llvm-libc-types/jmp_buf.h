//===-- Definition of type jmp_buf ----------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef __LLVM_LIBC_TYPES_JMP_BUF_H__
#define __LLVM_LIBC_TYPES_JMP_BUF_H__

typedef struct {
#ifdef __x86_64__
  __UINT64_TYPE__ rbx;
  __UINT64_TYPE__ rbp;
  __UINT64_TYPE__ r12;
  __UINT64_TYPE__ r13;
  __UINT64_TYPE__ r14;
  __UINT64_TYPE__ r15;
  __UINTPTR_TYPE__ rsp;
  __UINTPTR_TYPE__ rip;
#else
#error "__jmp_buf not available for your target architecture."
#endif
} __jmp_buf;

typedef __jmp_buf jmp_buf[1];

#endif // __LLVM_LIBC_TYPES_JMP_BUF_H__
