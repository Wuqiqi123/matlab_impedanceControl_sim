/******************************************************************
 * File: raccel_sfcn_utils.h
 *
 * Abstract:
 *      functions for dynamically loading s-function mex files
 *
 * Copyright 2007-2017, The MathWorks, Inc. 
 ******************************************************************/

#ifndef __RACCEL_SFCN_UTILS_H__
#define __RACCEL_SFCN_UTILS_H__

void
raccelInitializeForMexSFcns();

void
raccelLoadSFcnMexFile(
    const char* sFcnName,
    const char* blockSID,
    SimStruct* simstruct,
    size_t childIdx
    );

#endif /* __RACCEL_SFCN_UTILS_H__ */
