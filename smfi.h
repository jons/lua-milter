/**
 */
#ifndef SMFI_FFI_H
#define SMFI_FFI_H

#include <stdarg.h>
#include <libmilter/mfapi.h>

int smfi_vsetmlreply (SMFICTX *ctx, char *rcode, char *xcode, int nargs, va_list ap);

#endif
