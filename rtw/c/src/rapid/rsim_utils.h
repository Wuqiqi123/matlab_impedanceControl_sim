/*
 * Copyright 2007-2014 The MathWorks, Inc.
 *
 * File: rsim_utils.h
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

#ifndef __RSIM_UTILS_H__
#define __RSIM_UTILS_H__


/*==========*
 * Typedefs *
 *==========*/


/* Information associated with parameters of a given data type */
typedef struct {
    /* data attributes */
    int  dataType;
    bool complex;
    int  dtTransIdx;
    size_t elSize; /* for debugging */
    
    /* data */
    size_t nEls;
    void *rVals;
    void *iVals;
    void *vals;
} DTParamInfo;

/* Optionally one for the model */
typedef struct {
    int     errFlag;
    size_t  numParams;   /* total number of params                   */
    double  checksum[4]; /* model checksum                           */

    size_t nTrans;    
    DTParamInfo *dtParamInfo;
} PrmStructData;

extern void rt_RapidReadMatFileAndUpdateParams(const SimStruct *S);

 


#endif /* __RSIM_UTILS_H__ */

/* LocalWords:  raccel RSIM rsim Blockath
 */
