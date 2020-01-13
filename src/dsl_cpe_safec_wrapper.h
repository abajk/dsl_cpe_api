/******************************************************************************

                     Copyright 2018 - 2019 Intel Corporation

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

#ifndef _SAFEC_WRAPPER_H
#define _SAFEC_WRAPPER_H

#if defined (SAFEC_SUPPORT) && (SAFEC_SUPPORT == 3)
#include "safe_mem_lib.h"
#include "safe_str_lib.h"

#define cpe_control_memcpy_s memcpy_s
#define cpe_control_memset_s memset_s
#define cpe_control_strncpy_s strncpy_s

#else

#warning "Safe C library is not available!"

#include <stddef.h>	/* size_t */
static __inline__ size_t safec_wrapper_min(size_t a, size_t b)
{
   return a > b ? b : a;
}

#define cpe_control_memcpy_s(dest, destsz, src, srcsz) memcpy(dest, src, safec_wrapper_min(destsz,srcsz))
#define cpe_control_memset_s(dest, destsz, src, srcsz) memset(dest, src, safec_wrapper_min(destsz,srcsz))
#define cpe_control_strncpy_s(dest, destsz, src, srcsz) strncpy(dest, src, safec_wrapper_min(destsz,srcsz))

#endif /* defined (SAFEC_SUPPORT) && (SAFEC_SUPPORT == 3) */

#endif /* _SAFEC_WRAPPER_H */

