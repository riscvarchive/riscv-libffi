/* Area:	ffi_call
   Purpose:	Check return value int, with many arguments
   Limitations:	none.
   PR:		none.
   Originator:	From the original ffitest.c  */

/* { dg-do run } */
#include "ffitest.h"

#include <stdlib.h>
//#include <int.h>
#include <math.h>

static int ABI_ATTR many(int f1, int f2, int f3, int f4, int f5, int f6, int f7, int f8, int f9, int f10, int f11, int f12, int f13)
{
#if 0
  printf("%f %f %f %f %f %f %f %f %f %f %f %f %f\n",
	 (double) f1, (double) f2, (double) f3, (double) f4, (double) f5, 
	 (double) f6, (double) f7, (double) f8, (double) f9, (double) f10,
	 (double) f11, (double) f12, (double) f13);
#endif

  return f1+f2+f3+f4+f5+f6+f7+f8+f9+f10+f11+f12+f13;
}

int main (void)
{
  ffi_cif cif;
  ffi_type *args[13];
  void *values[13];
  int fa[13];
  int f, ff;
  int i;

  for (i = 0; i < 13; i++)
    {
      args[i] = &ffi_type_sint;
      values[i] = &fa[i];
      fa[i] = (int) i;
    }

    /* Initialize the cif */
    CHECK(ffi_prep_cif(&cif, ABI_NUM, 13,
		       &ffi_type_sint, args) == FFI_OK);

    ffi_call(&cif, FFI_FN(many), &f, values);

    ff =  many(fa[0], fa[1],
	       fa[2], fa[3],
	       fa[4], fa[5],
	       fa[6], fa[7],
	       fa[8], fa[9],
	       fa[10],fa[11],fa[12]);

    if (f == ff)
      exit(0);
    else
      abort();
}
