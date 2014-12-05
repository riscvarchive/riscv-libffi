/* -----------------------------------------------------------------------
   ffi64.c - 2014 Michael Knyszek
   
   RISC-V 64-bit Foreign Function Interface 

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
     * - Whether or not its a 128 bit type (only long double)
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
        
    }

    /* Set the return type flag */

    if (cif->abi == FFI_RV64_SOFT_FLOAT)
    {
        switch (cif->rtype->type)
        {
            case FFI_TYPE_VOID:
            case FFI_TYPE_POINTER:
            case FFI_TYPE_STRUCT:
            //case FFI_TYPE_LONG_DOUBLE:
                cif->flags += cif->rtype->type << (FFI_FLAG_BITS * 8);
                break;
            case FFI_TYPE_SINT64:
            case FFI_TYPE_UINT64:
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
            case FFI_TYPE_SINT64:
            case FFI_TYPE_UINT64:
                cif->flags += FFI_TYPE_UINT64 << (FFI_FLAG_BITS * 8);
                break;
            default:
                cif->flags += FFI_TYPE_INT << (FFI_FLAG_BITS * 8);
                break;
        }
    }

    return FFI_OK;
}
