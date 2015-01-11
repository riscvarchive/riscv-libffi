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

#include <ffi.h>
#include <ffi_common.h>

#include <stdlib.h>
#include <stdint.h>

/* ffi_prep_args is called by the assembly routine once stack space
   has been allocated for the function's arguments */

static void ffi_prep_args(char *stack, extended_cif *ecif, int bytes, int flags)
{
    int i;
    void **p_argv;
    char *argp;
    ffi_type **p_arg;
    
    argp = stack;

    memset(stack, 0, bytes);

    if (ecif->cif->rstruct_flag != 0)
    {
        *(ffi_arg *) argp = (ffi_arg) ecif->rvalue;
        argp += sizeof(ffi_arg);
    }

    p_argv = ecif->avalue;

    for (i = 0, p_arg = ecif->cif->arg_types; i < ecif->cif->nargs; i++, p_arg++)
    {
        size_t z;
        unsigned int a;

        /* Align if necessary. */
        a = (*p_arg)->alignment;
        if (a < sizeof(ffi_arg))
            a = sizeof(ffi_arg);

        if ((a - 1) & (unsigned long) argp)
        {
            argp = (char *) ALIGN(argp, a);
        }

        z = (*p_arg)->size;
        if (z <= sizeof(ffi_arg))
        {
            int type = (*p_arg)->type;
            z = sizeof(ffi_arg);

            /* The size of a pointer depends on the ABI */
            if (type == FFI_TYPE_POINTER)
            #ifdef __riscv64
                type = FFI_TYPE_SINT64;
            #else
                type = FFI_TYPE_SINT32;
            #endif
            
            if (i < 8 && (ecif->cif->abi == FFI_RV32_SOFT_FLOAT || ecif->cif->abi == FFI_RV64_SOFT_FLOAT))
            {
                switch (type)
                {
                    case FFI_TYPE_FLOAT:
                        type = FFI_TYPE_UINT32;
                        break;
                    case FFI_TYPE_DOUBLE:
                        type = FFI_TYPE_UINT64;
                        break;
                    default:
                        break;
                }
            }
            
            switch (type)
            {
                case FFI_TYPE_SINT8:
                    *(ffi_arg *)argp = *(SINT8 *)(* p_argv);
                    break;

                case FFI_TYPE_UINT8:
                    *(ffi_arg *)argp = *(UINT8 *)(* p_argv);
                    break;

                case FFI_TYPE_SINT16:
                    *(ffi_arg *)argp = *(SINT16 *)(* p_argv);
                    break;

                case FFI_TYPE_UINT16:
                    *(ffi_arg *)argp = *(UINT16 *)(* p_argv);
                    break;

                case FFI_TYPE_SINT32:
                    *(ffi_arg *)argp = *(SINT32 *)(* p_argv);
                    break;

                case FFI_TYPE_UINT32:
                    *(ffi_arg *)argp = *(UINT32 *)(* p_argv);
                    break;
                    
                case FFI_TYPE_SINT64:
                    *(ffi_arg *)argp = *(SINT64 *)(* p_argv);
                    break;

                case FFI_TYPE_UINT64:
                    *(ffi_arg *)argp = *(UINT64 *)(* p_argv);
                    break;

                /* This can only happen with 64bit slots. */
                case FFI_TYPE_FLOAT:
                    *(float *) argp = *(float *)(* p_argv);
                    break;

                /* Handle structures. */
                default:
                    memcpy(argp, *p_argv, (*p_arg)->size);
                    break;
            }
        }
        else
        {
            unsigned long end = (unsigned long) argp + z;
            unsigned long cap = (unsigned long) stack + bytes;
            
            /* Check if the data will fit within the register space.
               Handle it if it doesn't. */
            
            if (end <= cap)
                memcpy(argp, *p_argv, z);
            else
            {
                unsigned long portion = cap - (unsigned long)argp;
                
                memcpy(argp, *p_argv, portion);
                argp = stack;
                z -= portion;
                memcpy(argp, (void*)((unsigned long)(*p_argv) + portion), z);
            }
        }
        
        p_argv++;
        argp += z;
    }
}

/* This code traverses structure definitions 
   and generates the appropriate flags. */

static unsigned calc_riscv_struct_flags(int soft_float, ffi_type *arg, unsigned *loc, unsigned *arg_reg)
{
    unsigned flags = 0;
    unsigned index = 0;
    ffi_type *e;
    
    if (soft_float)
        return 0;
    
    while ((e = arg->elements[index]))
    {
        /* Align this object. */
        *loc = ALIGN(*loc, e->alignment);
        
        if (e->type == FFI_TYPE_DOUBLE)
        {
            /* Already aligned to FFI_SIZEOF_ARG. */
            *arg_reg = *loc / FFI_SIZEOF_ARG;
            
            if (*arg_reg > 7)
                break;
            
            flags += (FFI_TYPE_DOUBLE << (*arg_reg * FFI_FLAG_BITS));
            *loc += e->size;
        }
        else
            *loc += e->size;
        
        index++;
    }
    
    /* Next Argument register at alignment of FFI_SIZEOF_ARG. */
    *arg_reg = ALIGN(*loc, FFI_SIZEOF_ARG) / FFI_SIZEOF_ARG;
    
    return flags;
}

/* The flags output of this routine should match the various struct cases
   described in ffitarget.h */

static unsigned calc_riscv_return_struct_flags(int soft_float, ffi_type *arg)
{
    unsigned flags = 0;
    unsigned small = FFI_TYPE_SMALLSTRUCT;
    ffi_type *e;
    
    /* Returning structures under n32 is a tricky thing.
       A struct with only one or two floating point fields
       is returned in $f0 (and $f2 if necessary). Any other
       struct results at most 128 bits are returned in $2
       (the first 64 bits) and $3 (remainder, if necessary).
       Larger structs are handled normally. */
    
    if (arg->size > 2 * FFI_SIZEOF_ARG)
        return 0;
    
    if (arg->size > 8)
        small = FFI_TYPE_SMALLSTRUCT2;
    
    e = arg->elements[0];
    if (e->type == FFI_TYPE_DOUBLE)
        flags = FFI_TYPE_DOUBLE;
    else if (e->type == FFI_TYPE_FLOAT)
        flags = FFI_TYPE_FLOAT;
    
    if (flags && (e = arg->elements[1]))
    {
        if (e->type == FFI_TYPE_DOUBLE)
            flags += FFI_TYPE_DOUBLE << FFI_FLAG_BITS;
        else if (e->type == FFI_TYPE_FLOAT)
            flags += FFI_TYPE_FLOAT << FFI_FLAG_BITS;
        else
            return small;
        
        if (flags && (arg->elements[2]))
        {
            /* There are three arguments and the first two are
               floats! This must be passed the old way. */
            return small;
        }
        
        if (soft_float)
            flags += FFI_TYPE_STRUCT_SOFT;
    }
    else if (!flags)
        return small;
    
    return flags;
}

/* Perform machine dependent cif processing */

ffi_status ffi_prep_cif_machdep(ffi_cif *cif)
{
    int type;
    unsigned arg_reg = 0;
    unsigned loc = 0;
    unsigned count = (cif->nargs < 8) ? cif->nargs : 8;
    unsigned index = 0;
    
    unsigned int struct_flags = 0;
    int soft_float = cif->abi == FFI_RV64_SOFT_FLOAT || cif->abi == FFI_RV32_SOFT_FLOAT;;
    
    cif->flags = 0;
    
    if (cif->rtype->type == FFI_TYPE_STRUCT)
    {
        struct_flags = calc_riscv_return_struct_flags(soft_float, cif->rtype);
        if (struct_flags == 0)
        {
            /* This means that the structure is being passed as
               a hidden argument */
            arg_reg = 1;
            count = (cif->nargs < 7) ? cif->nargs : 7;
            cif->rstruct_flag = !0;
        }
        else
            cif->rstruct_flag = 0;
    }
    else
        cif->rstruct_flag = 0;
    
    /* Set the first 8 existing argument types in the flag bit string
     * 
     * We only describe the two argument types we care about:
     * - Whether or not its a float/double
     * - Whether or not its a struct
     * 
     * This is is two bits per argument accounting for the first 16 bits
     * of cif->flags.
     * 
     * The last 16 bits are just used to describe the return type
     * 
     * FFI_FLAG_BITS = 2
     */
    
    while (count-- > 0 && arg_reg < 8)
    {
        type = (cif->arg_types)[index]->type;
        
        /* Handle float argument types for soft float case */
        if (soft_float)
        {
            switch (type)
            {
                case FFI_TYPE_FLOAT:
                    type = FFI_TYPE_UINT32;
                    break;
                case FFI_TYPE_DOUBLE:
                    type = FFI_TYPE_UINT64;
                    break;
                default:
                    break;
            }
        }
        switch (type)
        {
            case FFI_TYPE_FLOAT:  // = 2 = 0b10
            case FFI_TYPE_DOUBLE: // = 3 = 0b11
                cif->flags += ((cif->arg_types)[index]->type << (arg_reg * FFI_FLAG_BITS));
                arg_reg++;
                break;
            case FFI_TYPE_STRUCT:
                loc = arg_reg * FFI_SIZEOF_ARG;
                cif->flags += calc_riscv_struct_flags(soft_float, (cif->arg_types)[index], &loc, &arg_reg);
                break;
            default:
                arg_reg++;
                break;
        }
        index++;
    }
    
    /* Set the return type flag */
    
    type = cif->rtype->type;
    
    /* Handle float return types for soft float case */
    if (soft_float)
    {
        switch (type)
        {
            case FFI_TYPE_FLOAT:
                type = FFI_TYPE_UINT32;
                break;
            case FFI_TYPE_DOUBLE:
                type = FFI_TYPE_UINT64;
                break;
            default:
                break;
        }
    }
    
    switch (type)
    {
        case FFI_TYPE_STRUCT:
            if (struct_flags != 0)
            {
                /* The structure is returned via some tricky mechanism */
                cif->flags += FFI_TYPE_STRUCT << (FFI_FLAG_BITS * 8);
                cif->flags += struct_flags << (4 + (FFI_FLAG_BITS * 8));
            }
            /* else the structure is returned through a hidden
               first argument. Do nothing, 'cause FFI_TYPE_VOID is 0 */
            break;
        case FFI_TYPE_VOID:
            /* Do nothing, 'cause FFI_TYPE_VOID is 0 */
            break;
        case FFI_TYPE_FLOAT:
        case FFI_TYPE_DOUBLE:
            cif->flags += cif->rtype->type << (FFI_FLAG_BITS * 8);
            break;        
        #ifndef __riscv64
        case FFI_TYPE_SINT64:
        case FFI_TYPE_UINT64:
            cif->flags += FFI_TYPE_UINT64 << (FFI_FLAG_BITS * 8);
            break;
        #endif
        default:
            cif->flags += FFI_TYPE_INT << (FFI_FLAG_BITS * 8);
            break;
    }

    return FFI_OK;
}

/* Low level routine for calling RV64 functions */
extern int ffi_call_asm(void (*)(char *, extended_cif *, int, int), 
                         extended_cif *, unsigned, unsigned, 
                         unsigned *, void (*)(void))
                         __attribute__((visibility("hidden")));

void ffi_call(ffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
    extended_cif ecif;

    ecif.cif = cif;
    ecif.avalue = avalue;

    /* If the return value is a struct and we don't have a return	*/
    /* value address then we need to make one		                */

    if ((rvalue == NULL) && (cif->rtype->type == FFI_TYPE_STRUCT))
        ecif.rvalue = alloca(cif->rtype->size);
    else
        ecif.rvalue = rvalue;
    
    ffi_call_asm(ffi_prep_args, &ecif, cif->bytes, cif->flags, ecif.rvalue, fn);
}

#if FFI_CLOSURES

extern void ffi_closure_asm(void) __attribute__((visibility("hidden")));

ffi_status ffi_prep_closure_loc(ffi_closure *closure, ffi_cif *cif, void (*fun)(ffi_cif*,void*,void**,void*), void *user_data, void *codeloc)
{
    unsigned int *tramp = (unsigned int *) &closure->tramp[0];
    
    uintptr_t fn = (uintptr_t) ffi_closure_asm;
    FFI_ASSERT(tramp == codeloc);
    
    /* Remove when more than just rv64 is supported */
    if (cif->abi != FFI_RV64)
        return FFI_BAD_ABI;
    
    if (cif->abi == FFI_RV32 || cif->abi == FFI_RV32_SOFT_FLOAT || fn < 0x7ffff000U)
    {
        /* auipc t0, 0 (i.e. t0 <- codeloc) */
        tramp[0] = 0x00000297;
        /* lui t1, %hi(fn) */
        tramp[1] = 0x00000337 | ((fn + 0x800) & 0xFFFFF000);
        /* jalr x0, t1, %lo(fn) */
        tramp[2] = 0x00030067 | ((fn & 0xFFF) << 20);
        /* nops */
        tramp[3] = 0x00000013;
        tramp[4] = 0x00000013;
        tramp[5] = 0x00000013;
    }
    else
    {
        /* auipc t0, 0 (i.e. t0 <- codeloc) */
        tramp[0] = 0x00000297;
        /* ld t1, 16(t0) */
        tramp[1] = 0x0102b303;
        /* jalr x0, t1, %lo(fn) */
        tramp[2] = 0x00030067;
        /* nop */
        tramp[3] = 0x00000013;
        /* fn */
        tramp[4] = fn;
        tramp[5] = fn >> 32;
    }
    
    closure->cif = cif;
    closure->fun = fun;
    closure->user_data = user_data;
    __builtin___clear_cache(codeloc, codeloc + FFI_TRAMPOLINE_SIZE);
    
    return FFI_OK;
}

static void copy_struct(char *target, unsigned offset, ffi_abi abi, ffi_type *type, int argn, unsigned arg_offset, ffi_arg *ar, ffi_arg *fpr, int soft_float)
{
    ffi_type **elt_typep = type->elements;
    
    while(*elt_typep)
    {
        ffi_type *elt_type = *elt_typep;
        unsigned o;
        char *tp;
        char *argp;
        char *fpp;
        
        o = ALIGN(offset, elt_type->alignment);
        arg_offset += o - offset;
        offset = o;
        argn += arg_offset / sizeof(ffi_arg);
        arg_offset = arg_offset % sizeof(ffi_arg);
        argp = (char *)(ar + argn);
        fpp = (char *)(argn >= 8 ? ar + argn : fpr + argn);
        tp = target + offset;
        
        if (elt_type->type == FFI_TYPE_DOUBLE && !soft_float)
            *(double *)tp = *(double *)fpp;
        else
            memcpy(tp, argp + arg_offset, elt_type->size);
        
        offset += elt_type->size;
        arg_offset += elt_type->size;
        elt_typep++;
        argn += arg_offset / sizeof(ffi_arg);
        arg_offset = arg_offset % sizeof(ffi_arg);
    }
}

/*
* Decodes the arguments to a function, which will be stored on the
* stack. AR is the pointer to the beginning of the integer
* arguments. FPR is a pointer to the area where floating point
* registers have been saved.
*
* RVALUE is the location where the function return value will be
* stored. CLOSURE is the prepared closure to invoke.
*
* This function should only be called from assembly, which is in
* turn called from a trampoline.
*
* Returns the function return flags.
*
*/
int ffi_closure_riscv_inner(ffi_closure *closure, void *rvalue, ffi_arg *ar, ffi_arg *fpr)
{
    ffi_cif *cif;
    void **avaluep;
    ffi_arg *avalue;
    ffi_type **arg_types;
    int i, avn, argn;
    int soft_float;
    ffi_arg *argp;
    
    cif = closure->cif;
    soft_float = cif->abi == FFI_RV64_SOFT_FLOAT || cif->abi == FFI_RV32_SOFT_FLOAT;
    avalue = alloca(cif->nargs * sizeof (ffi_arg));
    avaluep = alloca(cif->nargs * sizeof (ffi_arg));
    argn = 0;
    
    if (cif->rstruct_flag)
    {
        rvalue = (void *)ar[0];
        argn = 1;
    }
    
    i = 0;
    avn = cif->nargs;
    arg_types = cif->arg_types;
    
    while (i < avn)
    {
        if (arg_types[i]->type == FFI_TYPE_FLOAT || arg_types[i]->type == FFI_TYPE_DOUBLE || arg_types[i]->type == FFI_TYPE_LONGDOUBLE)
        {
            argp = (argn >= 8 || soft_float) ? ar + argn : fpr + argn;
            if ((arg_types[i]->type == FFI_TYPE_LONGDOUBLE) && ((uintptr_t)argp & (arg_types[i]->alignment-1)))
            {
                argp=(ffi_arg*)ALIGN(argp,arg_types[i]->alignment);
                argn++;
            }
            avaluep[i] = (char *) argp;
        }
        else
        {
            unsigned type = arg_types[i]->type;
            
            if (arg_types[i]->alignment > sizeof(ffi_arg))
                argn = ALIGN(argn, arg_types[i]->alignment / sizeof(ffi_arg));
            
            argp = ar + argn;
            
            /* The size of a pointer depends on the ABI */
            if (type == FFI_TYPE_POINTER)
                type = (cif->abi == FFI_RV64 || cif->abi == FFI_RV64_SOFT_FLOAT) ? FFI_TYPE_SINT64 : FFI_TYPE_SINT32;
            if (soft_float && type == FFI_TYPE_FLOAT)
                type = FFI_TYPE_UINT32;
            
            switch (type)
            {
                case FFI_TYPE_SINT8:
                    avaluep[i] = &avalue[i];
                    *(SINT8 *) &avalue[i] = (SINT8) *argp;
                    break;
                    
                case FFI_TYPE_UINT8:
                    avaluep[i] = &avalue[i];
                    *(UINT8 *) &avalue[i] = (UINT8) *argp;
                    break;
                    
                case FFI_TYPE_SINT16:
                    avaluep[i] = &avalue[i];
                    *(SINT16 *) &avalue[i] = (SINT16) *argp;
                    break;
                    
                case FFI_TYPE_UINT16:
                    avaluep[i] = &avalue[i];
                    *(UINT16 *) &avalue[i] = (UINT16) *argp;
                    break;
                    
                case FFI_TYPE_SINT32:
                    avaluep[i] = &avalue[i];
                    *(SINT32 *) &avalue[i] = (SINT32) *argp;
                    break;
                    
                case FFI_TYPE_UINT32:
                    avaluep[i] = &avalue[i];
                    *(UINT32 *) &avalue[i] = (UINT32) *argp;
                    break;
                    
                case FFI_TYPE_STRUCT:
                    if (argn < 8)
                    {
                        /* Allocate space for the struct as at least part of
                        it was passed in registers. */
                        avaluep[i] = alloca(arg_types[i]->size);
                        copy_struct(avaluep[i], 0, cif->abi, arg_types[i],
                        argn, 0, ar, fpr, soft_float);
                        break;
                    }
                    
                /* Else fall through. */
                default:
                    avaluep[i] = (char *) argp;
                    break;
            }
        }
        
        argn += ALIGN(arg_types[i]->size, sizeof(ffi_arg)) / sizeof(ffi_arg);
        i++;
    }
    
    /* Invoke the closure. */
    (closure->fun) (cif, rvalue, avaluep, closure->user_data);
    return cif->flags >> (FFI_FLAG_BITS * 8);
}

#endif /* FFI_CLOSURES */
