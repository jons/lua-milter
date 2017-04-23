/**
 */
#include <stdio.h>
#include <ffi.h>
#include <libmilter/mfapi.h>
#include "smfi.h"


int smfi_vsetmlreply (SMFICTX *ctx, char *rcode, char *xcode, int nargs, va_list ap)
{
  int i, retval;
  ffi_cif ci;
  ffi_type *t_ret = &ffi_type_sint;
  ffi_type *t_args[nargs+3];
  void *args[nargs+3];

  for (i = 0; i < nargs+3; i++)
    t_args[i] = &ffi_type_pointer;

  if (FFI_OK != ffi_prep_cif(&ci, FFI_DEFAULT_ABI, nargs, t_ret, t_args))
  {
    fprintf(stderr, "smfi_vsetmlreply: ffi_prep_cif failed\n");
    return MI_FAILURE;
  }
  args[0] = ctx;
  args[1] = rcode;
  args[2] = xcode;
  for (i = 0; i < nargs; i++)
  {
    args[i+3] = va_arg(ap, char *);
  }
  ffi_call(&ci, FFI_FN(smfi_setmlreply), &retval, args);
  return retval;
}
