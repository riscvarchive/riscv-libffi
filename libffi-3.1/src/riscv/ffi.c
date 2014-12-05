/* -----------------------------------------------------------------------
   ffi.c - 2014 Michael Knyszek
   
   RISC-V 32-bit Foreign Function Interface 

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

    if (ecif->cif->rtype->type == FFI_TYPE_STRUCT)
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

                /* This can only happen with 64bit slots.  */
                case FFI_TYPE_FLOAT:
                    *(float *) argp = *(float *)(* p_argv);
                    break;

                /* Handle structures.  */
                default:
                    memcpy(argp, *p_argv, (*p_arg)->size);
                    break;
            }
        }
        else
        {
            memcpy(argp, *p_argv, z);
        }
        
        p_argv++;
        argp += z;
    }
}

/* Perform machine dependent cif processing */

ffi_status ffi_prep_cif_machdep(ffi_cif *cif)
{
    int i;
    unsigned int prev_dbl_size = 0;
    cif->flags = 0;
    
    /* Set the first 8 existing argument types in the flag bit string
     * 
     * We only describe the two argument types we care about:
     * - Whether or not its a float
     * - Whether or not its a 64 bit type
     * 
     * This is is two bits per argument accounting for the first 16 bits
     * of cif->flags.
     * 
     * The last 16 bits are just used to describe the return type
     * 
     * FFI_FLAG_BITS = 2
     */
    
    for(i = 0; i < cif->nargs && i < 8; ++i)
    {
        #ifdef __riscv64
        switch ((cif->arg_types)[i]->type)
        {
            case FFI_TYPE_FLOAT:
            case FFI_TYPE_DOUBLE:
                cif->flags += 1 << (FFI_FLAG_BITS * i);
                
                prev_dbl_size = 0;
                break;
                
            /*case FFI_TYPE_LONG_DOUBLE:
                if (i == 7) break;
                if (!prev_dbl_size && i % 2 != 0) ++i;
                
                cif->flags += 3 << (FFI_FLAG_BITS * i);
                
                ++i;
                prev_dbl_size = 1;
                break;
            */    
            default:
                prev_dbl_size = 0;
                break;
        }
        #else
        switch ((cif->arg_types)[i]->type)
        {
            case FFI_TYPE_FLOAT:
                cif->flags += 1 << (FFI_FLAG_BITS * i);
                
                prev_dbl_size = 0;
                break;
                
            case FFI_TYPE_DOUBLE:
                if (i == 7) break;
                if (!prev_dbl_size && i % 2 != 0) ++i;
                
                cif->flags += 3 << (FFI_FLAG_BITS * i);
                ++i;
                
                prev_dbl_size = 1;
                break;
                
            case FFI_TYPE_SINT64:
            case FFI_TYPE_UINT64:
                if (i == 7) break;
                if (!prev_dbl_size && i % 2 != 0) ++i;
                
                cif->flags += 2 << (FFI_FLAG_BITS * i);
                ++i;
                
                prev_dbl_size = 1;
                break;
                
            default:
                prev_dbl_size = 0;
                break;
        }
        #endif
    }

    /* Set the return type flag */

    if (cif->abi == FFI_RV32_SOFT_FLOAT || cif->abi == FFI_RV64_SOFT_FLOAT)
    {
        switch (cif->rtype->type)
        {
            case FFI_TYPE_VOID:
            case FFI_TYPE_POINTER:
            case FFI_TYPE_STRUCT:
            //case FFI_TYPE_LONG_DOUBLE:
                cif->flags += cif->rtype->type << (FFI_FLAG_BITS * 8);
                break;
        // long long is the same size as a word in riscv64
        #ifndef __riscv64
            case FFI_TYPE_SINT64:
            case FFI_TYPE_UINT64:
        #endif
            case FFI_TYPE_DOUBLE:
                cif->flags += FFI_TYPE_UINT64 << (FFI_FLAG_BITS * 8);
                break;
            case FFI_TYPE_FLOAT:
            default:
                cif->flags += FFI_TYPE_INT << (FFI_FLAG_BITS * 8);
                break;
        }
    }
    else
    {   
        switch (cif->rtype->type)
        {
            case FFI_TYPE_VOID:
            case FFI_TYPE_POINTER:
            case FFI_TYPE_STRUCT:
            case FFI_TYPE_FLOAT:
            case FFI_TYPE_DOUBLE:
            //case FFI_TYPE_LONG_DOUBLE:
                cif->flags += cif->rtype->type << (FFI_FLAG_BITS * 8);
                break;
        // long long is the same size as a word in riscv64
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
    }

    return FFI_OK;
}

/* Low level routine for calling RV32 functions */
extern int ffi_call_rv32(void (*)(char *, extended_cif *, int, int), 
                         extended_cif *, unsigned, unsigned, 
                         unsigned *, void (*)(void));

/* Low level routine for calling RV64 functions */
extern int ffi_call_rv64(void (*)(char *, extended_cif *, int, int), 
                         extended_cif *, unsigned, unsigned, 
                         unsigned *, void (*)(void));

void ffi_call(ffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
    extended_cif ecif;

    ecif.cif = cif;
    ecif.avalue = avalue;

    /* If the return value is a struct and we don't have a return	*/
    /* value address then we need to make one		        */

    if ((rvalue == NULL) && (cif->rtype->type == FFI_TYPE_STRUCT))
        ecif.rvalue = alloca(cif->rtype->size);
    else
        ecif.rvalue = rvalue;

    switch (cif->abi) 
    {
        case FFI_RV32:
        case FFI_RV32_SOFT_FLOAT:
            ffi_call_rv32(ffi_prep_args, &ecif, cif->bytes, cif->flags, ecif.rvalue, fn);
            break;
        case FFI_RV64:
        case FFI_RV64_SOFT_FLOAT:
            ffi_call_rv64(ffi_prep_args, &ecif, cif->bytes, cif->flags, ecif.rvalue, fn);
            break;
        default:
            FFI_ASSERT(0);
            break;
    }
}
