/* Copyright 2011-2013 The MathWorks, Inc. */
#ifndef __RTIOSTREAM_UTILS_H__
#define __RTIOSTREAM_UTILS_H__

/* Shipping C interface to the libmwrtiostreamutils shared library. */

#include "tmwtypes.h"

#ifndef EXTERN_C
#ifdef __cplusplus
/* support C++ call sites */
#define EXTERN_C extern "C"
#else
/* support C call sites */
#define EXTERN_C extern
#endif
#endif

/* detect shipping call site */
#ifndef LIBMWRTIOSTREAMUTILS_API
/* default definition */
#define LIBMWRTIOSTREAMUTILS_API
#include "rtiostream_loadlib.h" /* libH_type */
#else
#include "rtiostream_loadlib_h.hpp" /* libH_type */
#endif

/* load an RtIOStream shared library */
EXTERN_C LIBMWRTIOSTREAMUTILS_API int rtIOStreamLoadLib(libH_type * pLibH, const char * libName);

/* unload an RtIOStream shared library */
EXTERN_C LIBMWRTIOSTREAMUTILS_API int rtIOStreamUnloadLib(libH_type * pLibH);

/* Blocks until all requested outgoing data is sent.
 *
 * Times out after sentTimeout seconds, indicated
 * by timeoutOccurred being set to 1.
 *
 * Returns non-zero RtIOStream error code in case of error or timeout. */
EXTERN_C LIBMWRTIOSTREAMUTILS_API int rtIOStreamLibBlockingSend(const libH_type *pLibH, 
                           const int streamID, 
                           const uint8_T * src, 
                           const size_t sendSize, 
                           const int sendTimeout, 
                           int * timeoutOccurred);

/* Blocks until all requested incoming data is received.
 *
 * Times out after recvTimeout seconds, indicated
 * by timeoutOccurred being set to 1.
 *
 * Returns non-zero RtIOStream error code in case of error or timeout. */
EXTERN_C LIBMWRTIOSTREAMUTILS_API int rtIOStreamLibBlockingRecv(const libH_type *pLibH, 
                           const int streamID, 
                           uint8_T * dst, 
                           const size_t recvSize, 
                           const int recvTimeout, 
                           int * timeoutOccurred);

#endif
