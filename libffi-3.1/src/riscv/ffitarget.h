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

#ifndef __riscv64
# error We currently only support RV64.
#endif

#ifdef __LP64__
# define FFI_SIZEOF_ARG 8
#else
# define FFI_SIZEOF_ARG 4
#endif

#define FFI_FLAG_BITS 2

#ifndef LIBFFI_ASM

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

#else

#ifdef __riscv64
  #define REG_S sd
  #define REG_L ld
#else
  #define REG_S sw
  #define REG_L lw
#endif

#endif /* LIBFFI_ASM */

#define FFI_ARGS_D FFI_TYPE_DOUBLE
#define FFI_ARGS_F FFI_TYPE_FLOAT
#define FFI_ARGS_DD ((FFI_TYPE_DOUBLE << FFI_FLAG_BITS) + FFI_TYPE_DOUBLE)
#define FFI_ARGS_FF ((FFI_TYPE_FLOAT << FFI_FLAG_BITS) + FFI_TYPE_FLOAT)
#define FFI_ARGS_FD ((FFI_TYPE_DOUBLE << FFI_FLAG_BITS) + FFI_TYPE_FLOAT)
#define FFI_ARGS_DF ((FFI_TYPE_FLOAT << FFI_FLAG_BITS) + FFI_TYPE_DOUBLE)
#define FFI_TYPE_SMALLSTRUCT FFI_TYPE_UINT8
#define FFI_TYPE_SMALLSTRUCT2 FFI_TYPE_SINT8
#define FFI_TYPE_STRUCT_D (FFI_TYPE_STRUCT + (FFI_ARGS_D << 4))
#define FFI_TYPE_STRUCT_F (FFI_TYPE_STRUCT + (FFI_ARGS_F << 4))
#define FFI_TYPE_STRUCT_DD (FFI_TYPE_STRUCT + (FFI_ARGS_DD << 4))
#define FFI_TYPE_STRUCT_FF (FFI_TYPE_STRUCT + (FFI_ARGS_FF << 4))
#define FFI_TYPE_STRUCT_FD (FFI_TYPE_STRUCT + (FFI_ARGS_FD << 4))
#define FFI_TYPE_STRUCT_DF (FFI_TYPE_STRUCT + (FFI_ARGS_DF << 4))
#define FFI_TYPE_STRUCT_SMALL (FFI_TYPE_STRUCT + (5 << 4))
#define FFI_TYPE_STRUCT_SMALL2 (FFI_TYPE_STRUCT + (6 << 4))
#define FFI_TYPE_STRUCT_D_SOFT (FFI_TYPE_STRUCT_D + 256)
#define FFI_TYPE_STRUCT_F_SOFT (FFI_TYPE_STRUCT_F + 256)
#define FFI_TYPE_STRUCT_DD_SOFT (FFI_TYPE_STRUCT_DD + 256)
#define FFI_TYPE_STRUCT_FF_SOFT (FFI_TYPE_STRUCT_FF + 256)
#define FFI_TYPE_STRUCT_FD_SOFT (FFI_TYPE_STRUCT_FD + 256)
#define FFI_TYPE_STRUCT_DF_SOFT (FFI_TYPE_STRUCT_DF + 256)
#define FFI_TYPE_STRUCT_SOFT 16

/* ---- Definitions for closures ----------------------------------------- */

#define FFI_CLOSURES 1
#define FFI_TRAMPOLINE_SIZE 24
#define FFI_NATIVE_RAW_API 0
#define FFI_EXTRA_CIF_FIELDS unsigned rstruct_flag

#endif

