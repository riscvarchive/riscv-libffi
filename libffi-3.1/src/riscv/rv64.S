/* -----------------------------------------------------------------------
   rv64.S - Michael Knyszek 2014
   
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

# Can't resolve expressions, workaround is
# explicit enumeration of values

# FFI_SIZEOF_ARG = 8 in this case

#define FFI_SIZEOF_ARG_X0 0
#define FFI_SIZEOF_ARG_X1 8
#define FFI_SIZEOF_ARG_X2 16
#define FFI_SIZEOF_ARG_X3 24
#define FFI_SIZEOF_ARG_X4 32
#define FFI_SIZEOF_ARG_X5 40
#define FFI_SIZEOF_ARG_X6 48
#define FFI_SIZEOF_ARG_X7 56
#define FFI_SIZEOF_ARG_X8 64

#define ARG_MASK 65535

#define FFI_FLAG_BITS_X0 0
#define FFI_FLAG_BITS_X1 2
#define FFI_FLAG_BITS_X2 4
#define FFI_FLAG_BITS_X3 6
#define FFI_FLAG_BITS_X4 8
#define FFI_FLAG_BITS_X5 10
#define FFI_FLAG_BITS_X6 12
#define FFI_FLAG_BITS_X7 14

# Stack pointer needs to be 16-byte aligned, so frame size is rounded up
# FFI_SIZEOF_ARG should be 4 in the 32-bit case

    .text
    .align  2
    .globl  ffi_call_rv64
    .type   ffi_call_rv64, @function
ffi_call_rv64:   
    ### Prologue
    
    # a0 - ffi_prep_args pointer
    # a1 - extended_cif pointer
    # a2 - bytes
    # a3 - flags
    # a4 - rvalue
    # a5 - function ptr
    
    add     sp, sp, -FFI_SIZEOF_ARG_X6   # move stack pointer by frame size
                                         # must be 16-byte aligned 
    
    # stack ptr points to first argument on stack,
    # but there should be no arguments on the stack
    
    sd      flags,   FFI_SIZEOF_ARG_X0(sp)   # save flags
    sd      rvalue,  FFI_SIZEOF_ARG_X1(sp)   # save return value pointer
    sd      fn,      FFI_SIZEOF_ARG_X2(sp)   # save function pointer
    sd      fp,      FFI_SIZEOF_ARG_X3(sp)   # save frame pointer
    sd      ra,      FFI_SIZEOF_ARG_X4(sp)   # save return address
    
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
    
    ld      t0, 0(fp)                  # load the flags word
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
    ld      a0, FFI_SIZEOF_ARG_X0(sp) # load argument
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
    ld      a1, FFI_SIZEOF_ARG_X1(sp) # load argument
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
    ld      a2, FFI_SIZEOF_ARG_X2(sp) # load argument
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
    ld      a3, FFI_SIZEOF_ARG_X3(sp) # load argument
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
    ld      a4, FFI_SIZEOF_ARG_X4(sp) # load argument
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
    ld      a5, FFI_SIZEOF_ARG_X5(sp) # load argument
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
    ld      a6, FFI_SIZEOF_ARG_X6(sp) # load argument
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
    ld      a7, FFI_SIZEOF_ARG_X7(sp) # load argument
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

    ld      a0, FFI_SIZEOF_ARG_X0(sp) # load argument
    ld      a1, FFI_SIZEOF_ARG_X1(sp) # load argument
    ld      a2, FFI_SIZEOF_ARG_X2(sp) # load argument
    ld      a3, FFI_SIZEOF_ARG_X3(sp) # load argument
    ld      a4, FFI_SIZEOF_ARG_X4(sp) # load argument
    ld      a5, FFI_SIZEOF_ARG_X5(sp) # load argument
    ld      a6, FFI_SIZEOF_ARG_X6(sp) # load argument
    ld      a7, FFI_SIZEOF_ARG_X7(sp) # load argument

#endif

call_it:    
    # Load the function pointer
    ld      t4, FFI_SIZEOF_ARG_X2(fp)

    # When the return value pointer is NULL, assume no return value.
    ld      t1, FFI_SIZEOF_ARG_X1(fp)
    beq     t1, zero, return_void

    # is the return type an int? if not, and if we have an FPU,
    # jump ahead
#ifndef __riscv_soft_float
    ori     t3, zero, FFI_TYPE_INT
    bne     t2, t3, return_float
#endif
    
    jalr    t4 # call the function
    
    # We reload the return value because it was in a temp reg and
    # there was just a function call.
    ld      t0, FFI_SIZEOF_ARG_X1(fp)
    sd      a0, 0(t0)
    j       epilogue

#ifndef __riscv_soft_float
return_float:
    # is the return type a float? if not, jump ahead
    ori     t3, zero, FFI_TYPE_FLOAT
    bne     t2, t3, return_double
    
    jalr    t4 # call the function
    
    # We reload the return value because it was in a temp reg and
    # there was just a function call.
    ld      t0,  FFI_SIZEOF_ARG_X1(fp)
    fsw     fa0, FFI_SIZEOF_ARG_X0(t0)
    j       epilogue

return_double:
    # is the return type a double? if not, give up
    ori     t3, zero, FFI_TYPE_DOUBLE
    bne     t2, t3, return_void
    
    jalr    t4 # call the function
    
    # We reload the return value because it was in a temp reg and
    # there was just a function call.
    ld      t0,  FFI_SIZEOF_ARG_X1(fp)
    fsd     fa0, FFI_SIZEOF_ARG_X0(t0)
    j       epilogue
#endif
 
return_void:   
    jalr    t4 # call the function
    
epilogue:   
    add     sp, fp, zero    
    ld      fp, FFI_SIZEOF_ARG_X3(sp)  # Restore frame pointer
    ld      ra, FFI_SIZEOF_ARG_X4(sp)  # Restore return address
    add     sp, sp, FFI_SIZEOF_ARG_X8  # Fix stack pointer
    jr      ra
    
    .size   ffi_call_rv64, .-ffi_call_rv64
