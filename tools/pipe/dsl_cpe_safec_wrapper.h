/******************************************************************************

                     Copyright 2018 - 2019 Intel Corporation

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

#ifndef _SAFEC_WRAPPER_H
#define _SAFEC_WRAPPER_H

#if defined (SAFEC_SUPPORT)
#include "safe_mem_lib.h"
#include "safe_str_lib.h"

#if (SAFEC_SUPPORT == 1) || (SAFEC_SUPPORT == 3)

#define cpe_control_pipe_memcpy_s(dest, destsz, src, srcsz) memcpy_s(dest, destsz, src, srcsz)
#define cpe_control_pipe_strncpy_s(dest, destsz, src, srcsz) strncpy_s(dest, destsz, src, srcsz)

#else

#warning "Safe C library version is unknown!"

#endif /* if (SAFEC_SUPPORT == 1) elif (SAFEC_SUPPORT == 3) */

#else

#warning "Safe C library is not available!"

#include <stddef.h>	/* size_t */
static __inline__ size_t safec_wrapper_min(size_t a, size_t b)
{
   return a > b ? b : a;
}

#define cpe_control_pipe_memcpy_s(dest, destsz, src, srcsz) memcpy(dest, src, safec_wrapper_min(destsz,srcsz))
#define cpe_control_pipe_strncpy_s(dest, destsz, src, srcsz) strncpy(dest, src, safec_wrapper_min(destsz,srcsz))

#endif /* defined (SAFEC_SUPPORT) */

#endif /* _SAFEC_WRAPPER_H */
