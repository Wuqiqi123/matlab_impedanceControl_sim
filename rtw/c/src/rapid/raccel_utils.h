/*
 * Copyright 2007-2015 The MathWorks, Inc.
 *
 * File: raccel.h
 *
 *
 * Abstract:
 *	Data structures used in rapid accelerator, for To and From File 
 *      blocks, and for parameter handling.
 *
 * Requires include files
 *	tmwtypes.h
 *	simstruc_type.h
 * Note including simstruc.h before rsim.h is sufficient because simstruc.h
 * includes both tmwtypes.h and simstruc_types.h.
 */

#ifndef __RACCEL_UTILS_H__
#define __RACCEL_UTILS_H__



    /*==========*
     * Typedefs *
     *==========*/

    /* 
     * Information associated with either a non-struct data type (in which
     * case rVals and iVals hold the values of all parameters of that data-
     * type) or information associated with a struct-param leaf. 
     */
    typedef struct {
        /* data attributes */
        int dataType;
        bool complex;
        int dtTransIdx;
        size_t  elSize; 
    
        /* data */
        size_t  nEls;
        void *rVals;
        void *iVals;
        void *vals;

        /* model parameter? */
        bool modelParam;

        /* 
         * is ParamInfo associated with a non-struct data-type,
         * or with a struct-param leaf?
         */
        bool structLeaf;

        /* memory address of the parameter (for struct-param leaves only) */
        void *prmAddr;
    } ParamInfo;


    /* Optionally one for the model */
    typedef struct {
        int     errFlag;
        double  checksum[4]; /* model checksum                           */

        /* 
         * nTrans is the number of datatypes in the RTP
         * paramInfo will point to array of size nNonStructDataTypes+
         * nStructLeaves (there is one ParamInfo structure for each leaf
         * of every parameter).
         */
        size_t nTrans;
        size_t nNonStructDataTypes;
        size_t nStructLeaves;
        ParamInfo *paramInfo;
    } PrmStructData;

    extern const char *rt_RapidReadInportsMatFile(const char* inportFileName,
                                                  int* matFileFormat,
                                                  int isRaccel);              

    extern void rt_Interpolate_Datatype(void   *x1, void   *x2, void   *yout,
                                        real_T t,   real_T t1,  real_T t2,
                                        int    outputDType);

    extern int_T rt_getTimeIdx(real_T *timePtr, real_T t, int_T numTimePoints, 
                               int_T preTimeIdx, boolean_T interp, boolean_T timeHitOnly);

    extern void rt_RapidFreeGbls(int);

    extern const char *rt_RapidCheckRemappings(void);

    extern void rt_RapidReadMatFileAndUpdateParams(const SimStruct *S);

    extern void rt_ssGetBlockPath(SimStruct* S, int_T sysIdx, int_T blkIdx, char_T **path);
    extern void rt_ssSet_slErrMsg(SimStruct*, void *);
    extern void rt_ssReportDiagnosticAsWarning(SimStruct*, void *);

    extern void rt_RapidInitDiagLoggerDB(const char* dbhome, size_t);
    extern void rt_RapidReleaseDiagLoggerDB();


#endif /* __RACCEL_UTILS_H__ */

/* LocalWords:  raccel RSIM rsim Blockath Vals RTP
 */
