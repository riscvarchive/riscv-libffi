/* -----------------------------------------------------------------------
   ffi.c - Copyright (c) 2015 Michael Knyszek <mknyszek@berkeley.edu>
                         2015 Andrew Waterman <waterman@cs.berkeley.edu>
   Based on MIPS N32/64 port
   
   RISC-V Foreign Function Interface 

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

#define LIBFFI_ASM  
#include <fficonfig.h>
#include <ffi.h>
    
#define callback a0
#define ecif     a1
#define bytes    a2
#define flags    a3
#define rvalue   a4
#define fn       a5
#define fp       s0

#define FFI_SIZEOF_ARG_X0 (0 * FFI_SIZEOF_ARG)
#define FFI_SIZEOF_ARG_X1 (1 * FFI_SIZEOF_ARG)
#define FFI_SIZEOF_ARG_X2 (2 * FFI_SIZEOF_ARG)
#define FFI_SIZEOF_ARG_X3 (3 * FFI_SIZEOF_ARG)
#define FFI_SIZEOF_ARG_X4 (4 * FFI_SIZEOF_ARG)
#define FFI_SIZEOF_ARG_X5 (5 * FFI_SIZEOF_ARG)
#define FFI_SIZEOF_ARG_X6 (6 * FFI_SIZEOF_ARG)
#define FFI_SIZEOF_ARG_X7 (7 * FFI_SIZEOF_ARG)
#define FFI_SIZEOF_ARG_X8 (8 * FFI_SIZEOF_ARG)

#define ARG_MASK 65535

#define FFI_FLAG_BITS_X0 (0 * FFI_FLAG_BITS)
#define FFI_FLAG_BITS_X1 (1 * FFI_FLAG_BITS)
#define FFI_FLAG_BITS_X2 (2 * FFI_FLAG_BITS)
#define FFI_FLAG_BITS_X3 (3 * FFI_FLAG_BITS)
#define FFI_FLAG_BITS_X4 (4 * FFI_FLAG_BITS)
#define FFI_FLAG_BITS_X5 (5 * FFI_FLAG_BITS)
#define FFI_FLAG_BITS_X6 (6 * FFI_FLAG_BITS)
#define FFI_FLAG_BITS_X7 (7 * FFI_FLAG_BITS)

# Stack pointer needs to be 16-byte aligned, so frame size is rounded up
#ifdef __riscv64
#  define SIZEOF_FRAME (6 * FFI_SIZEOF_ARG)
#else
#  define SIZEOF_FRAME (8 * FFI_SIZEOF_ARG)
#endif

    .text
    .align  2
    .globl  ffi_call_asm
    .type   ffi_call_asm, @function
ffi_call_asm:   
    ### Prologue
    
    # a0 - ffi_prep_args pointer
    # a1 - extended_cif pointer
    # a2 - bytes
    # a3 - flags
    # a4 - rvalue
    # a5 - function ptr
    
    add     sp, sp, -SIZEOF_FRAME   # move stack pointer by frame size
                                    # must be 16-byte aligned 
    
    # stack ptr points to first argument on stack,
    # but there should be no arguments on the stack
    
    REG_S   flags,   FFI_SIZEOF_ARG_X0(sp)   # save flags
    REG_S   rvalue,  FFI_SIZEOF_ARG_X1(sp)   # save return value pointer
    REG_S   fn,      FFI_SIZEOF_ARG_X2(sp)   # save function pointer
    REG_S   fp,      FFI_SIZEOF_ARG_X3(sp)   # save frame pointer
    REG_S   ra,      FFI_SIZEOF_ARG_X4(sp)   # save return address
    
    add     fp, sp, zero             # new frame pointer is updated stack pointer
    add     t4, callback, zero       # function ptr to prep_args

    # Allocate at least 8 words on the argstack
    ori     t0, zero, FFI_SIZEOF_ARG_X8
    bltu    bytes, t0, thirtytwo

    # Here it turns out that our total arg space is bigger than our
    # argument register space, so we set it according to bytes
    # and makes sure its aligned to 16 bytes
    
    add     t0, bytes, 15      # make sure it is aligned 
    andi    t0, t0,   -16      # to a 16 byte boundry

thirtytwo:
    sub     sp, sp, t0 # move the stack pointer to reflect the arg space
    
    # a0 is the stack with proper arg space allocated
    add     a0, sp, zero
    
    # a1 is ecif
    # a2 is bytes
    # a3 is flags

    jalr    t4  # call ffi_prep_args
    
    REG_L   t0, 0(fp)                  # load the flags word
    srli    t2, t0, 16                 # shift our return type into t4
    
    li      t1, ARG_MASK
    and     t0, t0, t1                 # mask out the arg types into t0

    # time to load the arguments for the call

#ifndef __riscv_soft_float

####################
## SET ARGUMENT 0 ##
####################

set_arg0:
    srli    t1, t0, FFI_FLAG_BITS_X0 # Shift to get the bits for this argument
    andi    t1, t1, 3               # Mask out the bits for this argument
    
    # when its zero, it means its just a word-sized int/ptr
    bne     t1, zero, set_arg0_float 
    REG_L   a0, FFI_SIZEOF_ARG_X0(sp) # load argument
    j       set_arg1
    
set_arg0_float:
    addi    t1, t1, -1
    
    # when its zero, it means its just a word-sized float
    bne     t1, zero, set_arg0_double
    flw     fa0, FFI_SIZEOF_ARG_X0(sp) # load argument
    j       set_arg1
    
set_arg0_double:
    # otherwise it must be a double we're dealing with
    fld     fa0, FFI_SIZEOF_ARG_X0(sp)

####################
## SET ARGUMENT 1 ##
####################

set_arg1:
    srli    t1, t0, FFI_FLAG_BITS_X1 # Shift to get the bits for this argument
    andi    t1, t1, 3               # Mask out the bits for this argument
    
    # when its zero, it means its just a word-sized int/ptr
    bne     t1, zero, set_arg1_float 
    REG_L   a1, FFI_SIZEOF_ARG_X1(sp) # load argument
    j       set_arg2
    
set_arg1_float:
    addi    t1, t1, -1
    
    # when its zero, it means its just a word-sized float
    bne     t1, zero, set_arg1_double
    flw     fa1, FFI_SIZEOF_ARG_X1(sp) # load argument
    j       set_arg2
    
set_arg1_double:
    # otherwise it must be a double we're dealing with
    fld     fa1, FFI_SIZEOF_ARG_X1(sp)

####################
## SET ARGUMENT 2 ##
####################

set_arg2:
    srli    t1, t0, FFI_FLAG_BITS_X2 # Shift to get the bits for this argument
    andi    t1, t1, 3               # Mask out the bits for this argument
    
    # when its zero, it means its just a word-sized int/ptr
    bne     t1, zero, set_arg2_float 
    REG_L   a2, FFI_SIZEOF_ARG_X2(sp) # load argument
    j       set_arg3
    
set_arg2_float:
    addi    t1, t1, -1
    
    # when its zero, it means its just a word-sized float
    bne     t1, zero, set_arg2_double
    flw     fa2, FFI_SIZEOF_ARG_X2(sp) # load argument
    j       set_arg3
    
set_arg2_double:
    # otherwise it must be a double we're dealing with
    fld     fa2, FFI_SIZEOF_ARG_X2(sp)

####################
## SET ARGUMENT 3 ##
####################

set_arg3:
    srli    t1, t0, FFI_FLAG_BITS_X3 # Shift to get the bits for this argument
    andi    t1, t1, 3               # Mask out the bits for this argument
    
    # when its zero, it means its just a word-sized int/ptr
    bne     t1, zero, set_arg3_float 
    REG_L   a3, FFI_SIZEOF_ARG_X3(sp) # load argument
    j       set_arg4
    
set_arg3_float:
    addi    t1, t1, -1
    
    # when its zero, it means its just a word-sized float
    bne     t1, zero, set_arg3_double
    flw     fa3, FFI_SIZEOF_ARG_X3(sp) # load argument
    j       set_arg4
    
set_arg3_double:
    # otherwise it must be a double we're dealing with
    fld     fa3, FFI_SIZEOF_ARG_X3(sp)

####################
## SET ARGUMENT 4 ##
####################

set_arg4:
    srli    t1, t0, FFI_FLAG_BITS_X4 # Shift to get the bits for this argument
    andi    t1, t1, 3               # Mask out the bits for this argument
    
    # when its zero, it means its just a word-sized int/ptr
    bne     t1, zero, set_arg4_float 
    REG_L   a4, FFI_SIZEOF_ARG_X4(sp) # load argument
    j       set_arg5
    
set_arg4_float:
    addi    t1, t1, -1
    
    # when its zero, it means its just a word-sized float
    bne     t1, zero, set_arg4_double
    flw     fa4, FFI_SIZEOF_ARG_X4(sp) # load argument
    j       set_arg5
    
set_arg4_double:
    # otherwise it must be a double we're dealing with
    fld     fa4, FFI_SIZEOF_ARG_X4(sp)

####################
## SET ARGUMENT 5 ##
####################

set_arg5:
    srli    t1, t0, FFI_FLAG_BITS_X5 # Shift to get the bits for this argument
    andi    t1, t1, 3               # Mask out the bits for this argument
    
    # when its zero, it means its just a word-sized int/ptr
    bne     t1, zero, set_arg5_float 
    REG_L   a5, FFI_SIZEOF_ARG_X5(sp) # load argument
    j       set_arg6
    
set_arg5_float:
    addi    t1, t1, -1
    
    # when its zero, it means its just a word-sized float
    bne     t1, zero, set_arg5_double
    flw     fa5, FFI_SIZEOF_ARG_X5(sp) # load argument
    j       set_arg6
    
set_arg5_double:
    # otherwise it must be a double we're dealing with
    fld     fa5, FFI_SIZEOF_ARG_X5(sp)

####################
## SET ARGUMENT 6 ##
####################

set_arg6:
    srli    t1, t0, FFI_FLAG_BITS_X6 # Shift to get the bits for this argument
    andi    t1, t1, 3               # Mask out the bits for this argument
    
    # when its zero, it means its just a word-sized int/ptr
    bne     t1, zero, set_arg6_float 
    REG_L   a6, FFI_SIZEOF_ARG_X6(sp) # load argument
    j       set_arg7
    
set_arg6_float:
    addi    t1, t1, -1
    
    # when its zero, it means its just a word-sized float
    bne     t1, zero, set_arg6_double
    flw     fa6, FFI_SIZEOF_ARG_X6(sp) # load argument
    j       set_arg7
    
set_arg6_double:
    # otherwise it must be a double we're dealing with
    fld     fa6, FFI_SIZEOF_ARG_X6(sp)

####################
## SET ARGUMENT 7 ##
####################

set_arg7:
    srli    t1, t0, FFI_FLAG_BITS_X7 # Shift to get the bits for this argument
    andi    t1, t1, 3               # Mask out the bits for this argument
    
    # when its zero, it means its just a word-sized int/ptr
    bne     t1, zero, set_arg7_float 
    REG_L   a7, FFI_SIZEOF_ARG_X7(sp) # load argument
    j       call_it
    
set_arg7_float:
    addi    t1, t1, -1
    
    # when its zero, it means its just a word-sized float
    bne     t1, zero, set_arg7_double
    flw     fa7, FFI_SIZEOF_ARG_X7(sp) # load argument
    j       call_it
    
set_arg7_double:
    # otherwise it must be a double we're dealing with
    fld     fa7, FFI_SIZEOF_ARG_X7(sp)
    
#else

## START RISCV SOFT-FLOAT LOADING ##

    # In the soft-float case, we have no primitive datatype
    # that has a size of >8 bytes. Therefore, we can 
    # just load everything quite easily and nicely.

    REG_L   a0, FFI_SIZEOF_ARG_X0(sp) # load argument
    REG_L   a1, FFI_SIZEOF_ARG_X1(sp) # load argument
    REG_L   a2, FFI_SIZEOF_ARG_X2(sp) # load argument
    REG_L   a3, FFI_SIZEOF_ARG_X3(sp) # load argument
    REG_L   a4, FFI_SIZEOF_ARG_X4(sp) # load argument
    REG_L   a5, FFI_SIZEOF_ARG_X5(sp) # load argument
    REG_L   a6, FFI_SIZEOF_ARG_X6(sp) # load argument
    REG_L   a7, FFI_SIZEOF_ARG_X7(sp) # load argument

#endif

call_it:    
    # Load the function pointer
    REG_L   t4, FFI_SIZEOF_ARG_X2(fp)

    # When the return value pointer is NULL, assume no return value.
    REG_L   t1, FFI_SIZEOF_ARG_X1(fp)
    beq     t1, zero, return_void

    # is the return type an int? if not, and if we have an FPU,
    # jump ahead
#ifndef __riscv_soft_float
    ori     t3, zero, FFI_TYPE_INT
    bne     t2, t3, return_float
#endif
    
    jalr    t4 # call the function
    
    # We reload the return pointer because it was in a temp reg and
    # there was just a function call.
    REG_L   t0, FFI_SIZEOF_ARG_X1(fp)
    REG_S   a0, 0(t0)
    j       epilogue

    # when we're in the soft float case, we'll just fall through here to the
    # soft float struct special cases
    
#ifndef __riscv_soft_float
return_float:
    # is the return type a float? if not, jump ahead
    ori     t3, zero, FFI_TYPE_FLOAT
    bne     t2, t3, return_double
    
    jalr    t4 # call the function
    
    # We reload the return pointer because it was in a temp reg and
    # there was just a function call.
    REG_L   t0,  FFI_SIZEOF_ARG_X1(fp)
    fsw     fa0, FFI_SIZEOF_ARG_X0(t0)
    j       epilogue

return_double:
    # is the return type a double? if not, give up
    ori     t3, zero, FFI_TYPE_DOUBLE
    bne     t2, t3, return_struct_d
    
    jalr    t4 # call the function
    
    # We reload the return pointer because it was in a temp reg and
    # there was just a function call.
    REG_L   t0,  FFI_SIZEOF_ARG_X1(fp)
    fsd     fa0, FFI_SIZEOF_ARG_X0(t0)
    j       epilogue

### Handle struct special cases (hard float)

  # Here the struct to return is less than
  # or equal to 2 pointer-words in size. We
  # need to specifically handle the floats/doubles.

return_struct_d:
    # is the return type a struct with a double? if not, give up
    ori     t3, zero, FFI_TYPE_STRUCT_D
    bne     t2, t3,   return_struct_f
    
    jalr    t4 # call the function
    
    # We reload the return pointer because it was in a temp reg and
    # there was just a function call.
    REG_L   t0,  FFI_SIZEOF_ARG_X1(fp)
    fsd     fa0, FFI_SIZEOF_ARG_X0(t0)
    j       epilogue
    
return_struct_f:
    # is the return type a struct with a float? if not, give up
    ori     t3, zero, FFI_TYPE_STRUCT_F
    bne     t2, t3,   return_struct_d_d
    
    jalr    t4 # call the function
    
    # We reload the return pointer because it was in a temp reg and
    # there was just a function call.
    REG_L   t0,  FFI_SIZEOF_ARG_X1(fp)
    fsw     fa0, FFI_SIZEOF_ARG_X0(t0)
    j       epilogue
    
return_struct_d_d:
    # is the return type a struct with two doubles? if not, give up
    ori     t3, zero, FFI_TYPE_STRUCT_DD
    bne     t2, t3,   return_struct_f_f
    
    jalr    t4 # call the function
    
    # We reload the return pointer because it was in a temp reg and
    # there was just a function call.
    REG_L   t0,  FFI_SIZEOF_ARG_X1(fp)
    fsd     fa0, FFI_SIZEOF_ARG_X0(t0)
    fsd     fa1, FFI_SIZEOF_ARG_X1(t0)
    j       epilogue
    
return_struct_f_f:
    # is the return type a struct with two floats? if not, give up
    ori     t3, zero, FFI_TYPE_STRUCT_FF
    bne     t2, t3,   return_struct_d_f
    
    jalr    t4 # call the function
    
    # We reload the return pointer because it was in a temp reg and
    # there was just a function call.
    REG_L   t0,  FFI_SIZEOF_ARG_X1(fp)
    fsw     fa0, FFI_SIZEOF_ARG_X0(t0)
    fsw     fa1, FFI_SIZEOF_ARG_X1(t0)
    j       epilogue
    
return_struct_d_f:
    # is the return type a struct with a double then float? if not, give up
    ori     t3, zero, FFI_TYPE_STRUCT_DF
    bne     t2, t3,   return_struct_f_d
    
    jalr    t4 # call the function
    
    # We reload the return pointer because it was in a temp reg and
    # there was just a function call.
    REG_L   t0,  FFI_SIZEOF_ARG_X1(fp)
    fsd     fa0, FFI_SIZEOF_ARG_X0(t0)
    fsw     fa1, FFI_SIZEOF_ARG_X1(t0)
    j       epilogue
    
return_struct_f_d:
    # is the return type a struct with a float then double? if not, give up
    ori     t3, zero, FFI_TYPE_STRUCT_FD
    bne     t2, t3,   return_struct_small
    
    jalr    t4 # call the function
    
    # We reload the return pointer because it was in a temp reg and
    # there was just a function call.
    REG_L   t0,  FFI_SIZEOF_ARG_X1(fp)
    fsw     fa0, FFI_SIZEOF_ARG_X0(t0)
    fsd     fa1, FFI_SIZEOF_ARG_X1(t0)
    j       epilogue
    
#else

### Handle struct special cases (soft float)

  # Here the struct to return is less than
  # or equal to 2 pointer-words in size. We
  # need to specifically handle the floats/doubles.

return_struct_d_soft:
    # is the return type a struct with a double? if not, give up
    ori     t3, zero, FFI_TYPE_STRUCT_D_SOFT
    bne     t2, t3,   return_struct_f_soft
    
    jalr    t4 # call the function
    
    # We reload the return pointer because it was in a temp reg and
    # there was just a function call.
    REG_L   t0,  FFI_SIZEOF_ARG_X1(fp)
    sd      a0,  FFI_SIZEOF_ARG_X0(t0)
    j       epilogue
    
return_struct_f_soft:
    # is the return type a struct with a float? if not, give up
    ori     t3, zero, FFI_TYPE_STRUCT_F_SOFT
    bne     t2, t3,   return_struct_d_d_soft
    
    jalr    t4 # call the function
    
    # We reload the return pointer because it was in a temp reg and
    # there was just a function call.
    REG_L   t0,  FFI_SIZEOF_ARG_X1(fp)
    sw      a0,  FFI_SIZEOF_ARG_X0(t0)
    j       epilogue
    
return_struct_d_d_soft:
    # is the return type a struct with two doubles? if not, give up
    ori     t3, zero, FFI_TYPE_STRUCT_DD_SOFT
    bne     t2, t3,   return_struct_f_f_soft
    
    jalr    t4 # call the function
    
    # We reload the return pointer because it was in a temp reg and
    # there was just a function call.
    REG_L   t0,  FFI_SIZEOF_ARG_X1(fp)
    sd      a0,  FFI_SIZEOF_ARG_X0(t0)
    sd      a1,  FFI_SIZEOF_ARG_X1(t0)
    j       epilogue
    
return_struct_f_f_soft:
    # is the return type a struct with two floats? if not, give up
    ori     t3, zero, FFI_TYPE_STRUCT_FF_SOFT
    bne     t2, t3,   return_struct_d_f_soft
    
    jalr    t4 # call the function
    
    # We reload the return pointer because it was in a temp reg and
    # there was just a function call.
    REG_L   t0,  FFI_SIZEOF_ARG_X1(fp)
    sw      a0,  FFI_SIZEOF_ARG_X0(t0)
    sw      a1,  FFI_SIZEOF_ARG_X1(t0)
    j       epilogue
    
return_struct_d_f_soft:
    # is the return type a struct with a double then float? if not, give up
    ori     t3, zero, FFI_TYPE_STRUCT_DF_SOFT
    bne     t2, t3,   return_struct_f_d_soft
    
    jalr    t4 # call the function
    
    # We reload the return pointer because it was in a temp reg and
    # there was just a function call.
    REG_L   t0,  FFI_SIZEOF_ARG_X1(fp)
    sd      a0,  FFI_SIZEOF_ARG_X0(t0)
    sw      a1,  FFI_SIZEOF_ARG_X1(t0)
    j       epilogue
    
return_struct_f_d_soft:
    # is the return type a struct with a float then double? if not, give up
    ori     t3, zero, FFI_TYPE_STRUCT_FD_SOFT
    bne     t2, t3,   return_struct_small
    
    jalr    t4 # call the function
    
    # We reload the return pointer because it was in a temp reg and
    # there was just a function call.
    REG_L   t0,  FFI_SIZEOF_ARG_X1(fp)
    sw      a0,  FFI_SIZEOF_ARG_X0(t0)
    sd      a1,  FFI_SIZEOF_ARG_X1(t0)
    j       epilogue
    
#endif    

### Handle struct special cases (tiny structs)

return_struct_small:
    # is the return type a struct with a float then double? if not, give up
    ori     t3, zero, FFI_TYPE_STRUCT_SMALL
    bne     t2, t3,   return_struct_small2
    
    jalr    t4 # call the function
    
    # We reload the return pointer because it was in a temp reg and
    # there was just a function call.
    REG_L   t0,  FFI_SIZEOF_ARG_X1(fp)
    REG_S   a0,  FFI_SIZEOF_ARG_X0(t0)
    j       epilogue
    
return_struct_small2:
    # is the return type a struct with a float then double? if not, give up
    ori     t3, zero, FFI_TYPE_STRUCT_SMALL2
    bne     t2, t3,   return_struct
    
    jalr    t4 # call the function
    
    # We reload the return pointer because it was in a temp reg and
    # there was just a function call.
    REG_L   t0,  FFI_SIZEOF_ARG_X1(fp)
    REG_S   a0,  FFI_SIZEOF_ARG_X0(t0)
    REG_S   a1,  FFI_SIZEOF_ARG_X1(t0)
    j       epilogue
    
### Any other struct is returned through memory
return_struct: 
return_void:   
    jalr    t4 # call the function
   
epilogue:   
    add     sp, fp, zero    
    REG_L   fp, FFI_SIZEOF_ARG_X3(sp)  # Restore frame pointer
    REG_L   ra, FFI_SIZEOF_ARG_X4(sp)  # Restore return address
    add     sp, sp, FFI_SIZEOF_ARG_X8  # Fix stack pointer
    jr      ra
    
    .size   ffi_call_asm, .-ffi_call_asm

    
/* ffi_closure_asm. Expects address of the passed-in ffi_closure in t0. */

#define SIZEOF_FRAME2 (20 * FFI_SIZEOF_ARG)
#define A7_OFF2 (19 * FFI_SIZEOF_ARG)
#define A6_OFF2 (18 * FFI_SIZEOF_ARG)
#define A5_OFF2 (17 * FFI_SIZEOF_ARG)
#define A4_OFF2 (16 * FFI_SIZEOF_ARG)
#define A3_OFF2 (15 * FFI_SIZEOF_ARG)
#define A2_OFF2 (14 * FFI_SIZEOF_ARG)
#define A1_OFF2 (13 * FFI_SIZEOF_ARG)
#define A0_OFF2 (12 * FFI_SIZEOF_ARG)
#define FA7_OFF2 (11 * FFI_SIZEOF_ARG)
#define FA6_OFF2 (10 * FFI_SIZEOF_ARG)
#define FA5_OFF2 (9 * FFI_SIZEOF_ARG)
#define FA4_OFF2 (8 * FFI_SIZEOF_ARG)
#define FA3_OFF2 (7 * FFI_SIZEOF_ARG)
#define FA2_OFF2 (6 * FFI_SIZEOF_ARG)
#define FA1_OFF2 (5 * FFI_SIZEOF_ARG)
#define FA0_OFF2 (4 * FFI_SIZEOF_ARG)
#define V1_OFF2 (3 * FFI_SIZEOF_ARG)
#define V0_OFF2 (2 * FFI_SIZEOF_ARG)
#define RA_OFF2 (1 * FFI_SIZEOF_ARG)

    .align 2
    .globl ffi_closure_asm
    .type ffi_closure_asm, @function
ffi_closure_asm:

    addi sp, sp, -SIZEOF_FRAME2
    REG_S ra, RA_OFF2(sp) # Save return address
    
    # Store all possible argument registers. If there are more than
    # fit in registers, then they were stored on the stack.
    REG_S a0, A0_OFF2(sp)
    REG_S a1, A1_OFF2(sp)
    REG_S a2, A2_OFF2(sp)
    REG_S a3, A3_OFF2(sp)
    REG_S a4, A4_OFF2(sp)
    REG_S a5, A5_OFF2(sp)
    REG_S a6, A6_OFF2(sp)
    REG_S a7, A7_OFF2(sp)
    
    # Store all possible float/double registers.
    fsd fa0, FA0_OFF2(sp)
    fsd fa1, FA1_OFF2(sp)
    fsd fa2, FA2_OFF2(sp)
    fsd fa3, FA3_OFF2(sp)
    fsd fa4, FA4_OFF2(sp)
    fsd fa5, FA5_OFF2(sp)
    fsd fa6, FA6_OFF2(sp)
    fsd fa7, FA7_OFF2(sp)
    
    # Call ffi_closure_riscv_inner to do the real work.
    move a0, t0 # Pointer to the ffi_closure
    addi a1, sp, V0_OFF2
    addi a2, sp, A0_OFF2
    addi a3, sp, FA0_OFF2
    call ffi_closure_riscv_inner
    
    # Return flags are in a0
    li t0, FFI_TYPE_SINT32
    bne a0, t0, cls_retint
    lw a0, V0_OFF2(sp)
    j cls_epilogue
    
cls_retint:
    li t0, FFI_TYPE_INT
    bne a0, t0, cls_retfloat
    REG_L a0, V0_OFF2(sp)
    j cls_epilogue
    
cls_retfloat:
    li t0, FFI_TYPE_FLOAT
    bne a0, t0, cls_retdouble
    flw fa0, V0_OFF2(sp)
    j cls_epilogue
    
cls_retdouble:
    li t0, FFI_TYPE_DOUBLE
    bne a0, t0, cls_retstruct_d
    fld fa0, V0_OFF2(sp)
    j cls_epilogue
    
cls_retstruct_d:
    li t0, FFI_TYPE_STRUCT_D
    bne a0, t0, cls_retstruct_f
    fld fa0, V0_OFF2(sp)
    j cls_epilogue
    
cls_retstruct_f:
    li t0, FFI_TYPE_STRUCT_F
    bne a0, t0, cls_retstruct_d_d
    flw fa0, V0_OFF2(sp)
    j cls_epilogue
    
cls_retstruct_d_d:
    li t0, FFI_TYPE_STRUCT_DD
    bne a0, t0, cls_retstruct_f_f
    fld fa0, V0_OFF2(sp)
    fld fa1, V1_OFF2(sp)
    j cls_epilogue
    
cls_retstruct_f_f:
    li t0, FFI_TYPE_STRUCT_FF
    bne a0, t0, cls_retstruct_d_f
    flw fa0, V0_OFF2(sp)
    flw fa1, V1_OFF2(sp)
    j cls_epilogue
    
cls_retstruct_d_f:
    li t0, FFI_TYPE_STRUCT_DF
    bne a0, t0, cls_retstruct_f_d
    fld fa0, V0_OFF2(sp)
    flw fa1, V1_OFF2(sp)
    j cls_epilogue
    
cls_retstruct_f_d:
    li t0, FFI_TYPE_STRUCT_FD
    bne a0, t0, cls_retstruct_small2
    flw fa0, V0_OFF2(sp)
    fld fa1, V1_OFF2(sp)
    j cls_epilogue
    
cls_retstruct_small2:
    REG_L a0, V0_OFF2(sp)
    REG_L a1, V1_OFF2(sp)
    
# Epilogue
cls_epilogue:
    REG_L ra, RA_OFF2(sp) # Restore return address
    addi sp, sp, SIZEOF_FRAME2
    ret
    
    .size ffi_closure_asm, .-ffi_closure_asm
