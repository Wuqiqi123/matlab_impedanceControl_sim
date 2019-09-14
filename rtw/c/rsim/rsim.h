/*
 * Copyright 1994-2007 The MathWorks, Inc.
 *
 * File: rsim.h
 *
 *
 * Abstract:
 *	Data structures used with the RSIM from file and from workspace block
 *      handling.
 *
 * Requires include files
 *	tmwtypes.h
 *	simstruc_type.h
 * Note including simstruc.h before rsim.h is sufficient because simstruc.h
 * includes both tmwtypes.h and simstruc_types.h.
 */

#ifndef __RSIM_H__
#define __RSIM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "common_utils.h"
#include "rsim_utils.h"

extern void rsimLoadSolverOpts(SimStruct* S);
extern void       rt_RSimRemapToFileName(char *fileName);
extern void       rt_RSimRemapFromFileName(char *fileName);

#ifdef __cplusplus
}
#endif

#endif /* __RSIM_H__ */
