/* -----------------------------------------------------------------*-C-*-
   ffitarget.h - 2014 Michael Knyszek
   
   Target configuration macros for RISC-V.

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   ``Software''), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED ``AS IS'', WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.

   ----------------------------------------------------------------------- */

#ifndef LIBFFI_TARGET_H
#define LIBFFI_TARGET_H

#ifndef LIBFFI_H
#error "Please do not include ffitarget.h directly into your source.  Use ffi.h instead."
#endif

#ifndef LIBFFI_ASM		

#define FFI_FLAG_BITS 2

typedef unsigned long ffi_arg; 
typedef   signed long ffi_sarg;

typedef enum ffi_abi {
  FFI_FIRST_ABI = 0,
  FFI_RV32,
  FFI_RV32_SOFT_FLOAT,
  FFI_RV64,
  FFI_RV64_SOFT_FLOAT,
  FFI_LAST_ABI,

#ifdef __riscv64
  #ifdef __riscv_soft_float
    FFI_DEFAULT_ABI = FFI_RV64_SOFT_FLOAT
  #else
    FFI_DEFAULT_ABI = FFI_RV64
  #endif
#else
  #ifdef __riscv_soft_float
    FFI_DEFAULT_ABI = FFI_RV32_SOFT_FLOAT
  #else
    FFI_DEFAULT_ABI = FFI_RV32
  #endif
#endif /* __riscv_soft_float */
} ffi_abi;

#endif /* LIBFFI_ASM */

/* ---- Definitions for closures ----------------------------------------- */

#define FFI_CLOSURES 0
#define FFI_TRAMPOLINE_SIZE 48
#define FFI_NATIVE_RAW_API 0

#endif

