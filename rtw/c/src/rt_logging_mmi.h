/* 
 *
 * Copyright 1994-2014 The MathWorks, Inc.
 *
 * File: rt_logging_mmi.h
 *
 * Abstract:
 */

#ifndef rt_logging_mmi_h
#define rt_logging_mmi_h

#include <stdlib.h>
#include "rtwtypes.h"
#include "builtin_typeid_types.h"
#include "rtw_matlogging.h"
#include "rtw_modelmap.h"

#ifdef __cplusplus
extern "C" {
#endif

const char_T * rt_FillStateSigInfoFromMMI(RTWLogInfo   *li,
                                          const char_T **errStatus);

void rt_CleanUpForStateLogWithMMI(RTWLogInfo *li);

#ifdef __cplusplus
}
#endif

#endif /*  rt_logging_mmi_h */
